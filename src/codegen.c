#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/regalloc.h"

#include <stdint.h>
#include <string.h>

/* ================================================================== */
/* x86-64 machine-code emission primitives                             */
/* ================================================================== */

static void emit_byte(Buffer *b, uint8_t val) {
    buffer_append(b, (const char *)&val, 1);
}

static void emit_int32(Buffer *b, int32_t val) {
    buffer_append(b, (const char *)&val, 4);
}

/* Emit a ModRM byte: (mod << 6) | (reg << 3) | rm */
static void emit_modrm(Buffer *b, int mod, int reg, int rm) {
    emit_byte(b, (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7)));
}

static void emit_rex_w(Buffer *b) { emit_byte(b, 0x48); }

/* mov %src, %dst   →  48 89 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_mov_rr(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x89);
    emit_modrm(b, 3, src, dst);
}

/* add %src, %dst  →  48 01 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_add_rr(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x01);
    emit_modrm(b, 3, src, dst);
}

/* sub %src, %dst  →  48 29 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_sub_rr(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x29);
    emit_modrm(b, 3, src, dst);
}

/* imul %src, %dst →  48 0F AF [ModRM: reg=dst, rm=src, mod=11] */
static void emit_imul_rr(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x0F);
    emit_byte(b, 0xAF);
    emit_modrm(b, 3, dst, src);
}

/* mov $imm32, %dst */
static void emit_mov_imm(Buffer *b, int dst_reg, int32_t imm) {
    emit_rex_w(b);
    emit_byte(b, 0xC7);
    emit_modrm(b, 3, 0, dst_reg);
    emit_int32(b, imm);
}

/* neg %dst */
static void emit_neg_r(Buffer *b, int dst_reg) {
    emit_rex_w(b);
    emit_byte(b, 0xF7);
    emit_modrm(b, 3, 3, dst_reg);
}

/* cqto */
static void emit_cqto(Buffer *b) { emit_rex_w(b); emit_byte(b, 0x99); }

/* idiv %rcx */
static void emit_idiv_rcx(Buffer *b) {
    emit_rex_w(b);
    emit_byte(b, 0xF7);
    emit_byte(b, 0xF9);
}

/* ================================================================== */
/* Stack-frame helpers                                                 */
/* ================================================================== */

static int spill_offset(int slot) { return -8 * (slot + 1); }

/* mov [rbp+off], %reg */
static void emit_store_spill(Buffer *b, int reg, int off) {
    emit_rex_w(b);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}

/* mov [rbp+off], %reg → load from spill slot */
static void emit_load_spill(Buffer *b, int reg, int off) {
    emit_rex_w(b);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}

/* ================================================================== */
/* Old-style stack-slot helpers (ra == NULL fallback)                  */
/* ================================================================== */

static int old_slot(int v) { return -(8 * (v + 1)); }

static void old_load(Buffer *b, int v, int reg) {
    int off = old_slot(v);
    emit_rex_w(b);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}

static void old_store(Buffer *b, int v, int reg) {
    int off = old_slot(v);
    emit_rex_w(b);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}

/* ================================================================== */
/* RA-aware value helpers                                              */
/* ================================================================== */

/* Ensure value `v` is in `dst_reg`.  Emits mov or load as needed. */
static void ensure_reg(Buffer *b, int v, int dst_reg, const RAResult *ra) {
    if (!ra || v < 0 || v >= ra->num_values) {
        /* Fallback: old stack-slot path. */
        old_load(b, v, dst_reg);
        return;
    }
    int vr = ra->reg[v];
    if (vr == dst_reg) return;
    if (vr >= 0 && vr < 16) {
        emit_mov_rr(b, dst_reg, vr);
    } else {
        emit_load_spill(b, dst_reg, spill_offset(ra->spill_slot[v]));
    }
}

/* Store to value v's spill slot if v is spilled (no-op if in register). */
static void spill_if_needed(Buffer *b, int v, int src_reg, const RAResult *ra) {
    if (!ra || v < 0 || v >= ra->num_values) {
        old_store(b, v, src_reg);
        return;
    }
    if (ra->reg[v] >= 0 && ra->reg[v] < 16) return;
    emit_store_spill(b, src_reg, spill_offset(ra->spill_slot[v]));
}

/* ================================================================== */
/* IR → x86-64 codegen                                                 */
/* ================================================================== */

void codegen(const IRModule *ir, EmitModule *out) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        const RAResult *ra = (const RAResult *)fn->ra;
        size_t start_offset = out->code.len;

        /* ---- Prologue ---- */
        int stack_size = ra ? ra->stack_size : 8 * fn->next_value_id;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);

        /* pushq %rbp */
        emit_byte(&out->code, 0x55);
        /* movq %rsp, %rbp */
        emit_rex_w(&out->code);
        emit_byte(&out->code, 0x89);
        emit_byte(&out->code, 0xE5);
        /* sub $N, %rsp */
        emit_rex_w(&out->code);
        emit_byte(&out->code, 0x81);
        emit_byte(&out->code, 0xEC);
        emit_int32(&out->code, stack_size);

        /* ---- Instruction loop ---- */
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            int dr = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                     ? ra->reg[inst->dst] : -1;

            switch (inst->op) {

            case IR_CONST: {
                if (dr >= 0) {
                    emit_mov_imm(&out->code, dr, inst->imm);
                } else {
                    emit_mov_imm(&out->code, REG_RAX, inst->imm);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_ADD: {
                if (dr >= 0) {
                    ensure_reg(&out->code, inst->a, dr, ra);
                    ensure_reg(&out->code, inst->b, REG_RCX, ra);
                    emit_add_rr(&out->code, dr, REG_RCX);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    old_load(&out->code, inst->b, REG_RCX);
                    emit_add_rr(&out->code, REG_RAX, REG_RCX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_SUB: {
                if (dr >= 0) {
                    ensure_reg(&out->code, inst->a, dr, ra);
                    ensure_reg(&out->code, inst->b, REG_RCX, ra);
                    emit_sub_rr(&out->code, dr, REG_RCX);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    old_load(&out->code, inst->b, REG_RCX);
                    emit_sub_rr(&out->code, REG_RAX, REG_RCX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_MUL: {
                if (dr >= 0) {
                    ensure_reg(&out->code, inst->a, dr, ra);
                    ensure_reg(&out->code, inst->b, REG_RCX, ra);
                    emit_imul_rr(&out->code, dr, REG_RCX);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    old_load(&out->code, inst->b, REG_RCX);
                    emit_imul_rr(&out->code, REG_RAX, REG_RCX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_DIV:
            case IR_MOD: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                emit_cqto(&out->code);
                emit_idiv_rcx(&out->code);
                if (inst->op == IR_DIV) {
                    if (dr >= 0 && dr != REG_RAX)
                        emit_mov_rr(&out->code, dr, REG_RAX);
                    spill_if_needed(&out->code, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                } else {
                    if (dr >= 0)
                        emit_mov_rr(&out->code, dr, REG_RDX);
                    spill_if_needed(&out->code, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                }
                break;
            }

            case IR_NEG: {
                if (dr >= 0) {
                    ensure_reg(&out->code, inst->a, dr, ra);
                    emit_neg_r(&out->code, dr);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    emit_neg_r(&out->code, REG_RAX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_ALLOCA:
                break;

            case IR_LOAD:
            case IR_COPY: {
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
                    /* Coalesced: same register → no-op. */
                    break;
                }
                if (dr >= 0) {
                    ensure_reg(&out->code, inst->a, dr, ra);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_STORE: {
                int sr = (ra && inst->b >= 0 && inst->b < ra->num_values)
                         ? ra->reg[inst->b] : -1;
                if (sr >= 0) {
                    spill_if_needed(&out->code, inst->a, sr, ra);
                }
                break;
            }

            case IR_LABEL:
            case IR_BR:
            case IR_CBR:
                die_at(inst->loc.file, inst->loc.line, inst->loc.col,
                       "control-flow codegen not yet supported (Slice 4)");
                break;

            case IR_RETURN: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);

                /* Epilogue */
                emit_rex_w(&out->code);
                emit_byte(&out->code, 0x81);
                emit_byte(&out->code, 0xC4);
                emit_int32(&out->code, stack_size);
                emit_byte(&out->code, 0x5D);
                emit_byte(&out->code, 0xC3);
                break;
            }

            } /* switch */
        } /* for insts */

        size_t fn_size = out->code.len - start_offset;
        emit_module_add_symbol(out, fn->name, start_offset, fn_size);
    }
}
