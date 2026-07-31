#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/regalloc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

/* cmp %src, %dst  →  48 39 [ModRM: reg=src, rm=dst, mod=11] (computes dst - src) */
static void emit_cmp_rr(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x39);
    emit_modrm(b, 3, src, dst);
}

/* test %r, %r  →  48 85 [ModRM: reg=r, rm=r, mod=11] — sets ZF if r == 0 */
static void emit_test_rr(Buffer *b, int r) {
    emit_rex_w(b);
    emit_byte(b, 0x85);
    emit_modrm(b, 3, r, r);
}

/* setcc %r (low 8 bits).  Encoded: 0F 9x [ModRM: mod=11, reg=0, rm=r].
 * Uses no REX-W (byte op); needs REX (no W) if r >= 4 to avoid AH/BH/CH/DH.
 * We emit REX (0x40) unconditionally for low-8 regs, and REX.B (0x41) for r8-r15. */
static void emit_setcc_r(Buffer *b, uint8_t cc_opcode, int r) {
    /* Need REX to force sil/dil/bpl/spl for regs 4-7, and REX.B for r8-r15. */
    uint8_t rex = 0x40 | ((r & 8) >> 3);
    emit_byte(b, rex);
    emit_byte(b, 0x0F);
    emit_byte(b, cc_opcode);
    emit_modrm(b, 3, 0, r & 7);
}

/* movzx dst, src8  (64-bit dst) →  48 0F B6 [ModRM: reg=dst, rm=src, mod=11] */
/* unused after xor+setcc scheme, kept for future use */
#if 0
static void emit_movzx_r8(Buffer *b, int dst, int src) {
    emit_rex_w(b);
    emit_byte(b, 0x0F);
    emit_byte(b, 0xB6);
    emit_modrm(b, 3, dst, src);
}
#endif

/* xor %r, %r  →  48 31 [ModRM: reg=r, rm=r, mod=11] — zero r */
static void emit_xor_rr(Buffer *b, int r) {
    emit_rex_w(b);
    emit_byte(b, 0x31);
    emit_modrm(b, 3, r, r);
}

/* jmp rel32  →  E9 rel32.  Returns offset of the rel32 field for patching. */
static size_t emit_jmp_rel32(Buffer *b) {
    emit_byte(b, 0xE9);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}

/* Jcc rel32  →  0F 8x rel32.  Returns offset of the rel32 field for patching. */
static size_t emit_jcc_rel32(Buffer *b, uint8_t cc_opcode) {
    emit_byte(b, 0x0F);
    emit_byte(b, cc_opcode);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
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
/* Comparison helpers                                                  */
/* ================================================================== */

/* Emit "cmp a, b" then "setcc dst; movzx dst, dst".
 * `a_reg` holds a's value, `b_reg` holds b's value.
 * cc_opcode is the setcc opcode (0x94 = sete, 0x95 = setne, etc.).
 * dst_reg must not be a_reg or b_reg (we zero it before cmp so flags survive). */
static void emit_cmp_produce(Buffer *b, int a_reg, int b_reg, int dst_reg,
                             uint8_t cc_opcode) {
    /* Zero dst first (xor sets flags — must happen BEFORE cmp). */
    emit_xor_rr(b, dst_reg);
    /* cmp %b_reg, %a_reg  → RAX - RCX form; setl triggers when a < b. */
    emit_cmp_rr(b, a_reg, b_reg);
    /* setcc writes only the low 8 bits; upper bits already zero. */
    emit_setcc_r(b, cc_opcode, dst_reg);
}

/* Map an IR comparison opcode to the setcc/Jcc suffix byte.  We use signed
 * variants: sete/setne/setl/setle/setg/setge. */
static uint8_t ir_cmp_to_setcc(int ir_op) {
    switch (ir_op) {
    case IR_EQ: return 0x94;   /* sete  */
    case IR_NE: return 0x95;   /* setne */
    case IR_LT: return 0x9C;   /* setl  */
    case IR_LE: return 0x9E;   /* setle */
    case IR_GT: return 0x9F;   /* setg  */
    case IR_GE: return 0x9D;   /* setge */
    default:    return 0x94;
    }
}

/* ================================================================== */
/* IR → x86-64 codegen                                                 */
/* ================================================================== */

/* Label patch record — a rel32 to be resolved once we know the label's
 * byte offset inside this function's code. */
typedef struct {
    size_t patch_off;     /* absolute offset of the rel32 field in out->code */
    int label;            /* target label id */
    size_t after_off;     /* offset of the instruction *following* the rel32,
                             used as base for rel32 = target - after */
} Patch;

void codegen(const IRModule *ir, EmitModule *out) {
    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        const RAResult *ra = (const RAResult *)fn->ra;
        size_t start_offset = out->code.len;

        /* ---- Prologue ---- */
        int stack_size = ra ? ra->stack_size : 8 * fn->next_value_id;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);

        emit_byte(&out->code, 0x55);              /* pushq %rbp */
        emit_rex_w(&out->code);
        emit_byte(&out->code, 0x89);
        emit_byte(&out->code, 0xE5);              /* movq %rsp, %rbp */
        emit_rex_w(&out->code);
        emit_byte(&out->code, 0x81);
        emit_byte(&out->code, 0xEC);
        emit_int32(&out->code, stack_size);       /* sub $N, %rsp */

        /* ---- Per-function label + patch tables ---- */
        /* label_off[label_id] = absolute code offset where the label lands,
         * or (size_t)-1 if not yet seen. */
        int nlabels = fn->next_label_id;
        size_t *label_off = NULL;
        if (nlabels > 0) {
            label_off = xmalloc(nlabels * sizeof(size_t));
            for (int L = 0; L < nlabels; L++) label_off[L] = (size_t)-1;
        }
        Patch *patches = NULL;
        size_t num_patches = 0, cap_patches = 0;

        #define ADD_PATCH(POFF, LBL, AFT) do { \
            if (num_patches >= cap_patches) { \
                cap_patches = cap_patches ? cap_patches * 2 : 8; \
                patches = xrealloc(patches, cap_patches * sizeof(Patch)); \
            } \
            patches[num_patches].patch_off = (POFF); \
            patches[num_patches].label = (LBL); \
            patches[num_patches].after_off = (AFT); \
            num_patches++; \
        } while (0)

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

            case IR_EQ:
            case IR_NE:
            case IR_LT:
            case IR_LE:
            case IR_GT:
            case IR_GE: {
                /* Materialize a in rax, b in rcx (scratch), compare, then
                 * write 0/1 into dst via setcc into a THIRD scratch (rdx)
                 * to avoid clobbering rax/rcx before/during cmp. */
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                uint8_t cc = ir_cmp_to_setcc(inst->op);
                emit_cmp_produce(&out->code, REG_RAX, REG_RCX, REG_RDX, cc);
                if (dr >= 0) {
                    if (dr != REG_RDX) emit_mov_rr(&out->code, dr, REG_RDX);
                } else {
                    spill_if_needed(&out->code, inst->dst, REG_RDX, ra);
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
                } else {
                    /* Fallback (no ra or spilled): load b's slot into rax,
                     * then store rax to a's slot. */
                    old_load(&out->code, inst->b, REG_RAX);
                    old_store(&out->code, inst->a, REG_RAX);
                }
                break;
            }

            case IR_LABEL: {
                if (inst->imm >= 0 && inst->imm < nlabels) {
                    label_off[inst->imm] = out->code.len;
                }
                break;
            }

            case IR_BR: {
                size_t patch = emit_jmp_rel32(&out->code);
                size_t after = out->code.len;
                ADD_PATCH(patch, inst->imm, after);
                break;
            }

            case IR_CBR: {
                /* CBR: a = cond, imm = true_label, b = false_label.
                 * Emit "test cond,cond; jne true_label; jmp false_label". */
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                emit_test_rr(&out->code, REG_RAX);
                /* jne true_label (0x85) */
                size_t p1 = emit_jcc_rel32(&out->code, 0x85);
                size_t a1 = out->code.len;
                ADD_PATCH(p1, inst->imm, a1);
                /* jmp false_label */
                size_t p2 = emit_jmp_rel32(&out->code);
                size_t a2 = out->code.len;
                ADD_PATCH(p2, inst->b, a2);
                break;
            }

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

        /* ---- Apply label patches ---- */
        for (size_t pi = 0; pi < num_patches; pi++) {
            Patch *p = &patches[pi];
            if (p->label < 0 || p->label >= nlabels ||
                label_off[p->label] == (size_t)-1) {
                fprintf(stderr, "fakecc: unresolved label %d in codegen\n", p->label);
                exit(1);
            }
            int64_t rel = (int64_t)label_off[p->label] - (int64_t)p->after_off;
            if (rel < INT32_MIN || rel > INT32_MAX) {
                fprintf(stderr, "fakecc: branch displacement out of range\n");
                exit(1);
            }
            int32_t rel32 = (int32_t)rel;
            memcpy(out->code.data + p->patch_off, &rel32, 4);
        }
        free(patches);
        free(label_off);

        size_t fn_size = out->code.len - start_offset;
        emit_module_add_symbol(out, fn->name, start_offset, fn_size);
    }
}
