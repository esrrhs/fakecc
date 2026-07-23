#include "fakecc/codegen.h"
#include "fakecc/common.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* x86-64 instruction encoding helpers                                 */
/* ------------------------------------------------------------------ */

static void emit_byte(Buffer *b, uint8_t val) {
    buffer_append(b, (const char *)&val, 1);
}

static void emit_int32(Buffer *b, int32_t val) {
    buffer_append(b, (const char *)&val, 4);
}

/* ------------------------------------------------------------------ */
/* Stack-slot offset helpers                                            */
/* ------------------------------------------------------------------ */

/* Each IRValue v is stored at [rbp - 8*(v+1)].
 * Returns the signed offset from %rbp (negative). */
static int slot_offset(int v) {
    return -(8 * (v + 1));
}

/* Emit a REX.W mov from rbp-offset to %rax.
 * Uses 8-bit displacement if offset fits, else 32-bit. */
static void emit_load_rax(Buffer *b, int v) {
    int off = slot_offset(v);
    emit_byte(b, 0x48);            /* REX.W */
    emit_byte(b, 0x8B);            /* mov r/m, r */
    if (off >= -128 && off <= 127) {
        emit_byte(b, 0x45);        /* ModRM: [rbp+disp8], %rax */
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_byte(b, 0x85);        /* ModRM: [rbp+disp32], %rax */
        emit_int32(b, off);
    }
}

/* Emit a REX.W mov from rbp-offset to %rcx. */
static void emit_load_rcx(Buffer *b, int v) {
    int off = slot_offset(v);
    emit_byte(b, 0x48);            /* REX.W */
    emit_byte(b, 0x8B);            /* mov r/m, r */
    if (off >= -128 && off <= 127) {
        emit_byte(b, 0x4D);        /* ModRM: [rbp+disp8], %rcx */
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_byte(b, 0x8D);        /* ModRM: [rbp+disp32], %rcx */
        emit_int32(b, off);
    }
}

/* Emit REX.W mov %rax → [rbp+off] (store). */
static void emit_store_rax(Buffer *b, int v) {
    int off = slot_offset(v);
    emit_byte(b, 0x48);            /* REX.W */
    emit_byte(b, 0x89);            /* mov r, r/m */
    if (off >= -128 && off <= 127) {
        emit_byte(b, 0x45);        /* ModRM: %rax, [rbp+disp8] */
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_byte(b, 0x85);        /* ModRM: %rax, [rbp+disp32] */
        emit_int32(b, off);
    }
}

/* Emit REX.W mov %rdx → [rbp+off] (store, for IR_MOD). */
static void emit_store_rdx(Buffer *b, int v) {
    int off = slot_offset(v);
    emit_byte(b, 0x48);            /* REX.W */
    emit_byte(b, 0x89);            /* mov r, r/m */
    if (off >= -128 && off <= 127) {
        emit_byte(b, 0x55);        /* ModRM: %rdx, [rbp+disp8] */
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_byte(b, 0x95);        /* ModRM: %rdx, [rbp+disp32] */
        emit_int32(b, off);
    }
}

/* ------------------------------------------------------------------ */
/* IR → machine code                                                   */
/* ------------------------------------------------------------------ */

void codegen(const IRModule *ir, EmitModule *out) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        size_t start_offset = out->code.len;

        /* Prologue: push %rbp; mov %rsp,%rbp; sub $N,%rsp */
        emit_byte(&out->code, 0x55);                 /* pushq %rbp */
        emit_byte(&out->code, 0x48);                 /* movq %rsp, %rbp */
        emit_byte(&out->code, 0x89);
        emit_byte(&out->code, 0xe5);

        /* Stack allocation: N = 8 * next_value_id, rounded up to 16-byte alignment */
        int stack_size = 8 * fn->next_value_id;
        if (stack_size % 16 != 0) {
            stack_size += 16 - (stack_size % 16);
        }
        /* sub $stack_size, %rsp = 48 81 EC <imm32> */
        emit_byte(&out->code, 0x48);
        emit_byte(&out->code, 0x81);
        emit_byte(&out->code, 0xEC);
        emit_int32(&out->code, stack_size);

        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            switch (inst->op) {

            case IR_CONST: {
                /* mov $imm, %rax; mov %rax, [rbp-8*(dst+1)] */
                emit_byte(&out->code, 0x48);         /* REX.W mov $imm32, %rax */
                emit_byte(&out->code, 0xC7);
                emit_byte(&out->code, 0xC0);
                emit_int32(&out->code, inst->imm);
                emit_store_rax(&out->code, inst->dst);
                break;
            }

            case IR_ADD:
            case IR_SUB:
            case IR_MUL: {
                /* load a→%rax, load b→%rcx, op %rcx,%rax, store %rax→slot */
                emit_load_rax(&out->code, inst->a);
                emit_load_rcx(&out->code, inst->b);
                switch (inst->op) {
                case IR_ADD:
                    /* add %rcx, %rax = 48 01 C8 */
                    emit_byte(&out->code, 0x48);
                    emit_byte(&out->code, 0x01);
                    emit_byte(&out->code, 0xC8);
                    break;
                case IR_SUB:
                    /* sub %rcx, %rax = 48 29 C8 */
                    emit_byte(&out->code, 0x48);
                    emit_byte(&out->code, 0x29);
                    emit_byte(&out->code, 0xC8);
                    break;
                case IR_MUL:
                    /* imul %rcx, %rax = 48 0F AF C1 */
                    emit_byte(&out->code, 0x48);
                    emit_byte(&out->code, 0x0F);
                    emit_byte(&out->code, 0xAF);
                    emit_byte(&out->code, 0xC1);
                    break;
                default: break; /* unreachable */
                }
                emit_store_rax(&out->code, inst->dst);
                break;
            }

            case IR_DIV:
            case IR_MOD: {
                /* load a→%rax, load b→%rcx, cqto, idiv %rcx */
                emit_load_rax(&out->code, inst->a);
                emit_load_rcx(&out->code, inst->b);
                /* cqto = 48 99 */
                emit_byte(&out->code, 0x48);
                emit_byte(&out->code, 0x99);
                /* idiv %rcx = 48 F7 F9 */
                emit_byte(&out->code, 0x48);
                emit_byte(&out->code, 0xF7);
                emit_byte(&out->code, 0xF9);
                if (inst->op == IR_DIV) {
                    emit_store_rax(&out->code, inst->dst); /* quotient in %rax */
                } else {
                    emit_store_rdx(&out->code, inst->dst); /* remainder in %rdx */
                }
                break;
            }

            case IR_NEG: {
                /* load a→%rax, neg %rax, store %rax→slot */
                emit_load_rax(&out->code, inst->a);
                /* neg %rax = 48 F7 D8 */
                emit_byte(&out->code, 0x48);
                emit_byte(&out->code, 0xF7);
                emit_byte(&out->code, 0xD8);
                emit_store_rax(&out->code, inst->dst);
                break;
            }

            case IR_ALLOCA:
                /* no-op: the slot is already reserved by the prologue's
                 * sub $N,%rsp (N = 8*next_value_id, 16-aligned). */
                break;

            case IR_LOAD: {
                /* load slot→%rax, store %rax→dst slot */
                emit_load_rax(&out->code, inst->a);
                emit_store_rax(&out->code, inst->dst);
                break;
            }

            case IR_STORE: {
                /* load value→%rax, store %rax→slot */
                emit_load_rax(&out->code, inst->b);
                emit_store_rax(&out->code, inst->a);
                break;
            }

            case IR_COPY: {
                /* load a→%rax, store %rax→dst slot (same as IR_LOAD shape) */
                emit_load_rax(&out->code, inst->a);
                emit_store_rax(&out->code, inst->dst);
                break;
            }

            case IR_LABEL:
            case IR_BR:
            case IR_CBR:
                /* Control-flow codegen not yet supported (Slice 4).
                 * The current frontend never produces these opcodes;
                 * hitting this means a logic error elsewhere. */
                die_at(inst->loc.file, inst->loc.line, inst->loc.col,
                       "control-flow codegen not yet supported (Slice 4)");
                break;

            case IR_RETURN: {
                /* load value→%rax, restore stack, pop %rbp, ret */
                emit_load_rax(&out->code, inst->a);
                /* add $stack_size, %rsp = 48 81 C4 <imm32> */
                emit_byte(&out->code, 0x48);
                emit_byte(&out->code, 0x81);
                emit_byte(&out->code, 0xC4);
                emit_int32(&out->code, stack_size);
                /* popq %rbp = 5d */
                emit_byte(&out->code, 0x5d);
                /* ret = c3 */
                emit_byte(&out->code, 0xc3);
                break;
            }

            } /* switch */
        } /* for insts */

        size_t fn_size = out->code.len - start_offset;
        emit_module_add_symbol(out, fn->name, start_offset, fn_size);
    }
}
