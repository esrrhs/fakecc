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

/* Emit a ModRM byte: (mod << 6) | (reg << 3) | rm — uses low 3 bits.
 * REX.R and REX.B (bit 3 of reg / rm) must be encoded in the preceding
 * REX prefix by callers via emit_rex_wrb(). */
static void emit_modrm(Buffer *b, int mod, int reg, int rm) {
    emit_byte(b, (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7)));
}

/* Emit REX prefix: 0x40 | (W<<3) | (R<<2) | (X<<1) | B.
 * R comes from the top bit of the ModRM.reg field's register.
 * B comes from the top bit of the ModRM.rm  field's register (or the
 * opcode-embedded register).
 * For 64-bit operations always pass W=1. */
static void emit_rex_wrb(Buffer *b, int w, int r_reg, int rm_reg) {
    int R = (r_reg >> 3) & 1;
    int B = (rm_reg >> 3) & 1;
    emit_byte(b, (uint8_t)(0x40 | (w << 3) | (R << 2) | B));
}

/* Compat shim — legacy call sites that already handle no-high-reg cases.
 * Prefer emit_rex_wrb for new code. */
static void emit_rex_w(Buffer *b) { emit_byte(b, 0x48); }

/* mov %src, %dst   →  REX 89 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_mov_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x89);
    emit_modrm(b, 3, src, dst);
}

/* add %src, %dst  →  REX 01 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_add_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x01);
    emit_modrm(b, 3, src, dst);
}

/* sub %src, %dst  →  REX 29 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_sub_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x29);
    emit_modrm(b, 3, src, dst);
}

/* imul %src, %dst →  REX 0F AF [ModRM: reg=dst, rm=src, mod=11] */
static void emit_imul_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0xAF);
    emit_modrm(b, 3, dst, src);
}

/* mov $imm32, %dst */
static void emit_mov_imm(Buffer *b, int dst_reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xC7);
    emit_modrm(b, 3, 0, dst_reg);
    emit_int32(b, imm);
}

/* neg %dst */
static void emit_neg_r(Buffer *b, int dst_reg) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xF7);
    emit_modrm(b, 3, 3, dst_reg);
}

/* not %dst  →  REX F7 [ModRM: /2, mod=11, rm=dst] */
static void emit_not_r(Buffer *b, int dst_reg) {
    emit_rex_wrb(b, 1, 0, dst_reg);
    emit_byte(b, 0xF7);
    emit_modrm(b, 3, 2, dst_reg);
}

/* and %src, %dst  →  REX 21 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_and_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x21);
    emit_modrm(b, 3, src, dst);
}

/* or %src, %dst  →  REX 09 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_or_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x09);
    emit_modrm(b, 3, src, dst);
}

/* xor %src, %dst  →  REX 31 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_bitxor_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x31);
    emit_modrm(b, 3, src, dst);
}

/* shl %cl, %dst  →  REX D3 [ModRM: /4, mod=11, rm=dst] (count must be in cl) */
static void emit_shl_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 4, dst);
}

/* shr %cl, %dst (logical)  →  REX D3 [ModRM: /5, mod=11, rm=dst] */
static void emit_shr_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 5, dst);
}

/* sar %cl, %dst (arithmetic)  →  REX D3 [ModRM: /7, mod=11, rm=dst] */
static void emit_sar_rcx(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, 0, dst);
    emit_byte(b, 0xD3);
    emit_modrm(b, 3, 7, dst);
}

/* cqto */
static void emit_cqto(Buffer *b) { emit_rex_w(b); emit_byte(b, 0x99); }

/* idiv %rcx */
static void emit_idiv_rcx(Buffer *b) {
    emit_rex_w(b);
    emit_byte(b, 0xF7);
    emit_byte(b, 0xF9);
}

/* cmp %src, %dst  →  REX 39 [ModRM: reg=src, rm=dst, mod=11] */
static void emit_cmp_rr(Buffer *b, int dst, int src) {
    emit_rex_wrb(b, 1, src, dst);
    emit_byte(b, 0x39);
    emit_modrm(b, 3, src, dst);
}

/* cmp $imm32, %reg  →  REX 81 [ModRM: /7, mod=11, rm=reg] imm32 */
static void emit_cmp_imm32(Buffer *b, int reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, reg);
    emit_byte(b, 0x81);
    emit_modrm(b, 3, 7, reg);
    emit_int32(b, imm);
}

/* add $imm32, %reg  →  REX 81 [ModRM: /0, mod=11, rm=reg] imm32 */
static void emit_add_imm32(Buffer *b, int reg, int32_t imm) {
    emit_rex_wrb(b, 1, 0, reg);
    emit_byte(b, 0x81);
    emit_modrm(b, 3, 0, reg);
    emit_int32(b, imm);
}

/* Patch a rel32 at `patch_off` to jump to `target_off`.  The rel32 is relative
 * to the byte after the 4-byte field (i.e. patch_off+4). */
static void patch_rel32(Buffer *b, size_t patch_off, size_t target_off) {
    int32_t rel = (int32_t)((int64_t)target_off - (int64_t)(patch_off + 4));
    memcpy(b->data + patch_off, &rel, 4);
}

/* test %r, %r  →  REX 85 [ModRM: reg=r, rm=r, mod=11] — sets ZF if r == 0 */
static void emit_test_rr(Buffer *b, int r) {
    emit_rex_wrb(b, 1, r, r);
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

/* xor %r, %r  →  REX 31 [ModRM: reg=r, rm=r, mod=11] — zero r */
static void emit_xor_rr(Buffer *b, int r) {
    emit_rex_wrb(b, 1, r, r);
    emit_byte(b, 0x31);
    emit_modrm(b, 3, r, r);
}

/* ---- SSE scalar emitters ---------------------------------------- */
/* XMM register ids are 0-15 (native ModRM codes, REX.B/R extended for   */
/* xmm8-xmm15).  These mirror the GP emitters but use the F2/F3 0F      */
/* opcode space.                                                         */

/* movsd xmm_dst, xmm_src  →  F2 0F 10 [ModRM: reg=dst, rm=src, mod=11].
 * Used for reg-reg XMM moves (copies low 64 bits; zeroes upper bits).
 * Doubles as the reg-reg move for float values too (low 32 bits hold the
 * float, bits 32-63 are zero after any movss-producing op). */
static void emit_sse_mov_rr(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm(b, 3, dst & 7, src & 7);
}

/* movsd xmm_dst, [rbp+off]  →  F2 0F 10 /r.  Loads 8 bytes (a spill slot
 * always holds 8 bytes, even for float). */
static void emit_sse_load_spill(Buffer *b, int dst, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, REG_RBP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst & 7, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst & 7, REG_RBP);
        emit_int32(b, off);
    }
}

/* movsd [rbp+off], xmm_src  →  F2 0F 11 /r.  Stores 8 bytes. */
static void emit_sse_store_spill(Buffer *b, int src, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, src, REG_RBP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, src & 7, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, src & 7, REG_RBP);
        emit_int32(b, off);
    }
}

/* SSE scalar arithmetic: add/sub/mul/div.
 *   op: 0x58=add 0x5C=sub 0x59=mul 0x5E=div
 * is_float (width 4) → single-precision (F3 prefix: addss etc.);
 * else double-precision (F2 prefix: addsd etc.).
 * Encoded <prefix> 0F <op> [ModRM: reg=dst, rm=src, mod=11]. */
static void emit_sse_arith(Buffer *b, int op, int dst, int src, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, op);
    emit_modrm(b, 3, dst & 7, src & 7);
}

/* Ordered comparison of two SSE scalars.  is_float → ucomiss (0F 2E /r);
 * else ucomisd (66 0F 2E /r).  Sets ZF/CF/PF. */
static void emit_sse_ucomi(Buffer *b, int a, int b_xmm, int is_float) {
    if (!is_float) emit_byte(b, 0x66);
    emit_rex_wrb(b, 0, a, b_xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2E);
    emit_modrm(b, 3, a & 7, b_xmm & 7);
}

/* cvtsi2sd xmm, r/m64  →  F2 REX.W 0F 2A /r.  Converts a 64-bit (or 32-bit
 * if !is_64) signed GP integer into a double in xmm. */
static void emit_sse_cvtsi2sd(Buffer *b, int xmm, int gp, int is_64) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, is_64 ? 1 : 0, xmm, gp);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2A);
    emit_modrm(b, 3, xmm & 7, gp & 7);
}

/* cvttsd2si r/m64, xmm  →  F2 REX.W 0F 2C /r.  Converts a double to a
 * signed GP integer, TRUNCATING toward zero (C semantics).  Note: the
 * non-truncating variant cvtsd2si honors the MXCSR rounding mode and would
 * round 3.5 to 4; cvtt* always truncates regardless. */
static void emit_sse_cvtsd2si(Buffer *b, int gp, int xmm, int is_64) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, is_64 ? 1 : 0, gp, xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2C);
    emit_modrm(b, 3, gp & 7, xmm & 7);
}

/* cvttss2si r/m64, xmm  →  F3 REX.W 0F 2C /r.  Converts a float to a
 * signed GP integer, TRUNCATING toward zero (C semantics).  Note: the
 * non-truncating variant cvtss2si honors the MXCSR rounding mode and would
 * round 3.5 to 4; cvtt* always truncates regardless. */
static void emit_sse_cvtss2si(Buffer *b, int gp, int xmm, int is_64) {
    emit_byte(b, 0xF3);
    emit_rex_wrb(b, is_64 ? 1 : 0, gp, xmm);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x2C);
    emit_modrm(b, 3, gp & 7, xmm & 7);
}

/* cvtss2sd xmm_dst, xmm_src  →  F3 0F 5A /r.  Widens a float (low 32 bits)
 * to a double in the low 64 bits. */
static void emit_sse_cvtss2sd(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF3);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x5A);
    emit_modrm(b, 3, dst & 7, src & 7);
}

/* cvtsd2ss xmm_dst, xmm_src  →  F2 0F 5A /r.  Narrows a double to a float
 * in the low 32 bits. */
static void emit_sse_cvtsd2ss(Buffer *b, int dst, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, src);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x5A);
    emit_modrm(b, 3, dst & 7, src & 7);
}

/* movq xmm_dst, gp_src  →  66 REX.W 0F 6E /r.  Loads 64 bits from a GP
 * register into the low 64 bits of an XMM register (used to materialize
 * float/double constants: movabs rax, bits; movq xmm, rax). */
static void emit_movq_xmm_gp(Buffer *b, int xmm, int gp) {
    emit_byte(b, 0x66);
    emit_rex_wrb(b, 1, xmm, gp);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x6E);
    emit_modrm(b, 3, xmm & 7, gp & 7);
}

/* mov %reg, [rsp+off]  →  REX.W 89 [mod reg rm=4 SIB=0x24] disp.  off must be
 * non-negative (the save area sits at/below rsp). */
static void emit_store_rsp_off(Buffer *b, int reg, int off) {
    emit_rex_wrb(b, 1, reg, REG_RSP);
    emit_byte(b, 0x89);
    if (off >= 0 && off <= 127) {
        emit_modrm(b, 1, reg, 4);
        emit_byte(b, 0x24);  /* SIB: base=rsp, index=none */
        emit_byte(b, (uint8_t)off);
    } else {
        emit_modrm(b, 2, reg, 4);
        emit_byte(b, 0x24);
        emit_int32(b, off);
    }
}

/* movsd [rsp], xmm_src  →  F2 0F 11 [mod=00 reg=src rm=4 SIB=0x24].  Used in
 * the prologue push-then-pop dance to push an XMM arg onto the stack. */
static void emit_sse_store_rsp(Buffer *b, int src) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, src, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    emit_modrm(b, 0, src & 7, 4);
    emit_byte(b, 0x24);  /* SIB: base=rsp, index=none */
}

/* movaps xmm_dst, [rsp+off]  →  0F 29 [mod dst rm=4 SIB=0x24] disp.  Saves an
 * FP arg register into the variadic save area (16-byte aligned slots). */
static void emit_sse_store_rsp_off(Buffer *b, int dst, int off) {
    emit_rex_wrb(b, 0, dst, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x29);
    if (off >= 0 && off <= 127) {
        emit_modrm(b, 1, dst, 4);
        emit_byte(b, 0x24);
        emit_byte(b, (uint8_t)off);
    } else {
        emit_modrm(b, 2, dst, 4);
        emit_byte(b, 0x24);
        emit_int32(b, off);
    }
}

/* movsd xmm_dst, [base+off] (load)  →  F2 0F 10 [mod dst rm=base] disp.
 * Generic: base is any register (encoding handles [rsp]-style SIB as needed). */
static void emit_sse_load_base_off(Buffer *b, int dst, int base, int off) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, base);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    if (off >= -128 && off <= 127) {
        if (base == REG_RSP) {
            emit_modrm(b, 1, dst, 4);
            emit_byte(b, 0x24);
            emit_byte(b, (uint8_t)(off & 0xFF));
        } else {
            emit_modrm(b, 1, dst, base);
            emit_byte(b, (uint8_t)(off & 0xFF));
        }
    } else {
        if (base == REG_RSP) {
            emit_modrm(b, 2, dst, 4);
            emit_byte(b, 0x24);
        } else {
            emit_modrm(b, 2, dst, base);
        }
        emit_int32(b, off);
    }
}

/* movsd xmm_dst, [rsp]  →  F2 0F 10 [mod=00 reg=dst rm=4 SIB=0x24].  Pops an
 * XMM arg off the stack in the prologue dance. */
static void emit_sse_load_rsp(Buffer *b, int dst) {
    emit_byte(b, 0xF2);
    emit_rex_wrb(b, 0, dst, REG_RSP);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm(b, 0, dst & 7, 4);
    emit_byte(b, 0x24);  /* SIB: base=rsp, index=none */
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

/* push %r  →  50+r  (with REX.B for r8-r15) */
static void emit_push_r(Buffer *b, int r) {
    if (r >= 8) emit_byte(b, 0x41);  /* REX.B */
    emit_byte(b, (uint8_t)(0x50 | (r & 7)));
}

/* pop %r  →  58+r  (with REX.B for r8-r15) */
static void emit_pop_r(Buffer *b, int r) {
    if (r >= 8) emit_byte(b, 0x41);
    emit_byte(b, (uint8_t)(0x58 | (r & 7)));
}

/* sub $imm32, %rsp — used for alignment padding */
static void emit_sub_rsp_imm32(Buffer *b, int32_t imm) {
    emit_rex_w(b);
    emit_byte(b, 0x81);
    emit_byte(b, 0xEC);
    emit_int32(b, imm);
}

/* add $imm32, %rsp */
static void emit_add_rsp_imm32(Buffer *b, int32_t imm) {
    emit_rex_w(b);
    emit_byte(b, 0x81);
    emit_byte(b, 0xC4);
    emit_int32(b, imm);
}

/* call rel32  →  E8 rel32.  Returns offset of the rel32 field for patching. */
static size_t emit_call_rel32(Buffer *b) {
    emit_byte(b, 0xE8);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}

/* call *%reg  →  FF /2 [ModRM: mod=11, reg=2, rm=reg].  Indirect call through
 * a register holding a function pointer.  REX.B (0x41) for reg >= 8. */
static void emit_indirect_call(Buffer *b, int reg) {
    if (reg >= 8) emit_byte(b, 0x41);  /* REX.B */
    emit_byte(b, 0xFF);
    emit_modrm(b, 3, 2, reg & 7);
}

/* PLT emission (emit_plt0 / emit_plt_entry) has been moved to the linker
 * (src/link.c): the PLT must be built after all modules are merged, since its
 * final code-buffer offset depends on the total merged .text length.  codegen
 * now records cross-module references as relocations instead. */

/* ================================================================== */
/* x87 FPU emission — long double (80-bit extended, width 16)          */
/*                                                                      */
/* Long doubles never occupy XMM registers.  Each lives in a dedicated
 * 16-byte stack slot (ld_off[v]) and moves through x87 st0: we `fldt` a
 * slot to load into st0, `fstpt` to store st0 back and pop.  Every ld
 * operation ends with st0 empty (the `*p` pop instructions + fstpt clear
 * the x87 stack).                                                      */
/*                                                                      */
/* x87 memory operands use the `/r` encoding: opcode DB (low) with the
 * reg field of ModRM selecting the operation (0=fild, 1=fisttp, 5=fldt,
 * 7=fstpt).  To keep ModRM simple we materialize each slot's address
 * into a GP scratch register first, then encode mod=00 rm=scratch.      */
/* ================================================================== */

/* fldt m80real  →  DB /5 [mod=00 reg=5 rm=scratch].  Load 10-byte
 * extended from [scratch] into st0 (pushes the x87 stack).  `scratch` must
 * not be RSP/RBP (their mod=00 encodings are special).  RCX is used. */
static void emit_x87_fldtRCX(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 5, REG_RCX & 7);
}

/* fstpt m80real  →  DB /7 [mod=00 reg=7 rm=scratch].  Store st0 to
 * [scratch] and pop the x87 stack. */
static void emit_x87_fstptRCX(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 7, REG_RCX & 7);
}

/* faddp / fsubp / fmulp / fdivp  →  DE C1 / DE E1 / DE C9 / DE F9.
 * Binary x87 op: st1 = st1 op st0, pop st0.  Encoded with mod=11, rm=st0. */
static void emit_x87_arith_pop(Buffer *b, int op) {
    emit_byte(b, 0xDE);
    emit_byte(b, (uint8_t)op);
}

/* fstpt [rsp]  →  DB /7 [mod=00 reg=7 rm=4 SIB=0x24].  Store st0 to a 16-byte
 * slot at [rsp] and pop.  Used to push long-double call args. */
static void emit_x87_fstpt_rsp(Buffer *b) {
    emit_byte(b, 0xDB);
    emit_modrm(b, 0, 7, 4); /* mod=00 rm=100 => SIB follows */
    emit_byte(b, 0x24);     /* SIB: base=rsp, index=none */
}

/* fcomip st0, st1  →  DF F1.  Compare st0 to st1, pop st0, set ZF/CF.
 * Used for long-double comparisons (IR_FCMP ld). */
static void emit_x87_fcomip(Buffer *b) {
    emit_byte(b, 0xDF);
    emit_byte(b, 0xF1);
}

/* Forward declarations: these helpers live later in the file (with the other
 * SSE/GP emitters) but are referenced by the ld load/store wrappers below. */
static void emit_ld_addr(Buffer *b, int reg, int ld_off);
static void emit_store_base_off(Buffer *b, int base, int reg, int off);
static void emit_load_base_off(Buffer *b, int dst, int base, int off);

/* Materialize long-double value `v`'s slot address into RCX, then fldt it
 * into st0.  `ld_off` is the prologue-computed rbp-relative slot array. */
static void emit_ld_load(Buffer *b, int v, const int *ld_off) {
    emit_ld_addr(b, REG_RCX, ld_off[v]);
    emit_x87_fldtRCX(b);
}

/* Load st0 into long-double value `v`'s slot and pop. */
static void emit_ld_store(Buffer *b, int v, const int *ld_off) {
    emit_ld_addr(b, REG_RCX, ld_off[v]);
    emit_x87_fstptRCX(b);
}

/* Load the 32-bit signed integer in GP register `src` into long-double value
 * `dst`'s slot via x87: mov src→[ld_slot]; fild dword [ld_slot]; fstpt the
 * result into [ld_slot].  int→long-double (IR_SITOFP ld).  Clobbers RAX/RCX. */
static void emit_ld_from_gp_int(Buffer *b, int src, int dst, const int *ld_off) {
    /* Store the int into the low 4 bytes of dst's 16-byte slot, fild it. */
    emit_ld_addr(b, REG_RCX, ld_off[dst]);
    emit_store_base_off(b, REG_RCX, src, 0); /* mov [rcx], src (32-bit) */
    emit_byte(b, 0xDB); emit_modrm(b, 0, 0, REG_RCX & 7); /* fild dword [rcx] */
    emit_x87_fstptRCX(b); /* fstpt [rcx] — st0 (the ld) → slot, pop */
}

/* SysV AMD64: first 6 integer args in rdi, rsi, rdx, rcx, r8, r9. */
static const int SYSV_ARG_REGS[6] = {
    REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9
};

/* ================================================================== */
/* Stack-frame helpers                                                 */
/* ================================================================== */

static int spill_offset(int slot) { return -8 * (slot + 1); }

/* mov [rbp+off], %reg */
static void emit_store_spill(Buffer *b, int reg, int off) {
    emit_rex_wrb(b, 1, reg, REG_RBP);
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
    emit_rex_wrb(b, 1, reg, REG_RBP);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, REG_RBP);
        emit_int32(b, off);
    }
}

/* lea dst_reg, [rbp+off]  →  REX 8D [ModRM: reg=dst, rm=rbp, mod=disp] */
static void emit_lea_rbp(Buffer *b, int dst, int off) {
    emit_rex_wrb(b, 1, dst, REG_RBP);
    emit_byte(b, 0x8D);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst, REG_RBP);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst, REG_RBP);
        emit_int32(b, off);
    }
}

/* lea dst_reg, [rip + disp32]  →  REX.W 8D [ModRM: mod=00 reg=dst rm=5] disp32.
 * Returns the file offset of the rel32 slot (caller records for later patch). */
static size_t emit_lea_rip(Buffer *b, int dst) {
    emit_rex_wrb(b, 1, dst, 0 /* B doesn't matter — rm=5 encodes RIP */);
    emit_byte(b, 0x8D);
    emit_modrm(b, 0, dst & 7, 5);
    size_t patch = b->len;
    emit_int32(b, 0);
    return patch;
}

/* Emit ModRM for `[reg]` addressing where `reg` is a full 4-bit register id
 * (0..15).  Handles the two x86 special cases:
 *   - rbp (reg&7 == 5): mod=00 rm=5 means [disp32], not [rbp]. Use mod=01 disp8=0.
 *   - rsp (reg&7 == 4): rm=4 means SIB follows.  Emit SIB=0x24 (base=rsp, no idx, no scale).
 */
static void emit_modrm_indirect(Buffer *b, int reg_field, int base) {
    int rm = base & 7;
    if (rm == 4) {
        emit_modrm(b, 0, reg_field & 7, 4);
        emit_byte(b, 0x24);  /* SIB: scale=0, index=none(4), base=rsp(4) */
    } else if (rm == 5) {
        emit_modrm(b, 1, reg_field & 7, 5);
        emit_byte(b, 0);
    } else {
        emit_modrm(b, 0, reg_field & 7, rm);
    }
}

/* SSE load/store via pointer: movsd/movss xmm, [ptr] and [ptr], xmm.
 * is_float (width 4) → single-precision (F3 prefix, 4 bytes);
 * else double-precision (F2 prefix, 8 bytes).  Used for float loads/stores
 * through a pointer (pinned float variables). */
static void emit_sse_load_via_ptr(Buffer *b, int dst, int ptr, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, dst, ptr);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x10);
    emit_modrm_indirect(b, dst, ptr);
}
static void emit_sse_store_via_ptr(Buffer *b, int ptr, int src, int is_float) {
    emit_byte(b, is_float ? 0xF3 : 0xF2);
    emit_rex_wrb(b, 0, src, ptr);
    emit_byte(b, 0x0F);
    emit_byte(b, 0x11);
    emit_modrm_indirect(b, src, ptr);
}

/* mov [ptr_reg], src_reg — store `width` low bytes of src to memory pointed by ptr. */
static void emit_store_via_ptr(Buffer *b, int ptr, int src, int width) {
    switch (width) {
    case 1: {
        uint8_t rex = 0x40 | ((src & 8) >> 1) | ((ptr & 8) >> 3);
        emit_byte(b, rex);
        emit_byte(b, 0x88);
        emit_modrm_indirect(b, src, ptr);
        break;
    }
    case 2:
        emit_byte(b, 0x66);
        emit_rex_wrb(b, 0, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    case 4:
        if (src >= 8 || ptr >= 8) emit_rex_wrb(b, 0, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    case 8:
    default:
        emit_rex_wrb(b, 1, src, ptr);
        emit_byte(b, 0x89);
        emit_modrm_indirect(b, src, ptr);
        break;
    }
}

/* Load from [ptr_reg] into dst_reg. If width < 8, sign/zero-extend. */
static void emit_load_via_ptr(Buffer *b, int dst, int ptr, int width, int is_unsigned) {
    switch (width) {
    case 1:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB6 : 0xBE);
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 2:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB7 : 0xBF);
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 4:
        if (is_unsigned) {
            if (dst >= 8 || ptr >= 8) emit_rex_wrb(b, 0, dst, ptr);
            emit_byte(b, 0x8B);
        } else {
            emit_rex_wrb(b, 1, dst, ptr);
            emit_byte(b, 0x63);
        }
        emit_modrm_indirect(b, dst, ptr);
        break;
    case 8:
    default:
        emit_rex_wrb(b, 1, dst, ptr);
        emit_byte(b, 0x8B);
        emit_modrm_indirect(b, dst, ptr);
        break;
    }
}

/* ---- Width-aware load/store to [rbp+off] ----
 * Currently unused: mem2reg promotes every scalar alloca, so IR_LOAD/STORE
 * never survives to codegen for Slice 7a. Kept for future non-scalar support. */
#if 0
static void emit_store_narrow(Buffer *b, int reg, int off, int width) {
    switch (width) {
    case 1: {
        /* mov byte ptr [rbp+off], reg8 — need REX (0x40) for reg 4-7 low byte,
         * REX.R for r8-r15. */
        uint8_t rex = 0x40 | ((reg & 8) >> 1);   /* REX.R = bit 2, reg>>3 */
        emit_byte(b, rex);
        emit_byte(b, 0x88);
        int mod = (off >= -128 && off <= 127) ? 1 : 2;
        emit_modrm(b, mod, reg & 7, REG_RBP);
        if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
        else emit_int32(b, off);
        break;
    }
    case 2: {
        /* 0x66 prefix + mov word ptr [rbp+off], reg16 */
        emit_byte(b, 0x66);
        emit_rex_wrb(b, 0, reg, REG_RBP);
        emit_byte(b, 0x89);
        int mod = (off >= -128 && off <= 127) ? 1 : 2;
        emit_modrm(b, mod, reg, REG_RBP);
        if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
        else emit_int32(b, off);
        break;
    }
    case 4: {
        /* mov dword ptr [rbp+off], reg32 — REX (no W) for r8-r15, otherwise no REX */
        if (reg >= 8) emit_rex_wrb(b, 0, reg, REG_RBP);
        emit_byte(b, 0x89);
        int mod = (off >= -128 && off <= 127) ? 1 : 2;
        emit_modrm(b, mod, reg, REG_RBP);
        if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
        else emit_int32(b, off);
        break;
    }
    default:
        emit_store_spill(b, reg, off);
        break;
    }
}

/* Load `width` bytes from [rbp+off] into `reg`, sign- or zero-extending to 64. */
static void emit_load_narrow(Buffer *b, int reg, int off, int width, int is_unsigned) {
    switch (width) {
    case 1: {
        /* movsx/movzx reg64, byte ptr [rbp+off] */
        emit_rex_wrb(b, 1, reg, REG_RBP);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB6 : 0xBE);
        int mod = (off >= -128 && off <= 127) ? 1 : 2;
        emit_modrm(b, mod, reg, REG_RBP);
        if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
        else emit_int32(b, off);
        break;
    }
    case 2: {
        emit_rex_wrb(b, 1, reg, REG_RBP);
        emit_byte(b, 0x0F);
        emit_byte(b, is_unsigned ? 0xB7 : 0xBF);
        int mod = (off >= -128 && off <= 127) ? 1 : 2;
        emit_modrm(b, mod, reg, REG_RBP);
        if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
        else emit_int32(b, off);
        break;
    }
    case 4: {
        if (is_unsigned) {
            /* mov reg32, dword ptr [rbp+off] — auto zero-extends to 64. */
            if (reg >= 8) emit_rex_wrb(b, 0, reg, REG_RBP);
            emit_byte(b, 0x8B);
            int mod = (off >= -128 && off <= 127) ? 1 : 2;
            emit_modrm(b, mod, reg, REG_RBP);
            if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
            else emit_int32(b, off);
        } else {
            /* movsxd reg64, dword ptr [rbp+off] */
            emit_rex_wrb(b, 1, reg, REG_RBP);
            emit_byte(b, 0x63);
            int mod = (off >= -128 && off <= 127) ? 1 : 2;
            emit_modrm(b, mod, reg, REG_RBP);
            if (mod == 1) emit_byte(b, (uint8_t)(off & 0xFF));
            else emit_int32(b, off);
        }
        break;
    }
    default:
        emit_load_spill(b, reg, off);
        break;
    }
}
#endif

/* movsx reg64, reg_small — sign-extend a register's low `src_w` bytes. */
static void emit_movsx_rr(Buffer *b, int dst, int src, int src_w) {
    if (src_w == 4) {
        /* movsxd dst64, src32 */
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x63);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 2) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xBF);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 1) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xBE);
        emit_modrm(b, 3, dst, src);
    } else {
        /* No-op: already 64-bit. */
        if (dst != src) emit_mov_rr(b, dst, src);
    }
}

/* movzx reg64, reg_small — zero-extend. */
static void emit_movzx_rr(Buffer *b, int dst, int src, int src_w) {
    if (src_w == 4) {
        /* mov dst32, src32 — auto zero-extends to 64. */
        if (dst >= 8 || src >= 8) emit_rex_wrb(b, 0, src, dst);
        emit_byte(b, 0x89);
        emit_modrm(b, 3, src, dst);
    } else if (src_w == 2) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xB7);
        emit_modrm(b, 3, dst, src);
    } else if (src_w == 1) {
        emit_rex_wrb(b, 1, dst, src);
        emit_byte(b, 0x0F);
        emit_byte(b, 0xB6);
        emit_modrm(b, 3, dst, src);
    } else {
        if (dst != src) emit_mov_rr(b, dst, src);
    }
}

/* Mask (zero-extend) a register's low `width` bytes; no-op for width==8. */
static void mask_to_width(Buffer *b, int reg, int width) {
    if (width >= 8 || width <= 0) return;
    emit_movzx_rr(b, reg, reg, width);
}

/* ================================================================== */
/* Old-style stack-slot helpers (ra == NULL fallback)                  */
/* ================================================================== */

static int old_slot(int v) { return -(8 * (v + 1)); }

static void old_load(Buffer *b, int v, int reg) {
    int off = old_slot(v);
    emit_rex_wrb(b, 1, reg, REG_RBP);
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
    emit_rex_wrb(b, 1, reg, REG_RBP);
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

/* True if SSA value `v` is a float (TY_FLOAT) value in this function.  Used
 * to pick the XMM register file (and XMM ABI slots) for float params, args,
 * and results.  Mirrors the helper in regalloc.c. */
static int value_is_float_class(const IRFunction *fn, int v) {
    if (v < 0) return 0;
    if (!fn->value_is_float || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_is_float[v];
}

/* True if SSA value `v` is a long double (TY_FLOAT width 16).  Long doubles
 * are NOT kept in XMM registers — they live in dedicated 16-byte stack slots
 * (ld_off[v]) and move through x87 st0.  This predicate gates the x87 path. */
static int value_is_ld(const IRFunction *fn, int v) {
    if (!value_is_float_class(fn, v)) return 0;
    if (!fn->value_width || fn->value_meta_cap <= 0) return 0;
    if (v >= fn->value_meta_cap) return 0;
    return fn->value_width[v] == 16;
}

/* Materialize the address of long-double value `v`'s 16-byte slot into GP
 * register `reg`.  ld_off[v] is the rbp-relative offset assigned in the
 * prologue (negative for locals/results, positive for stack-passed params). */
static void emit_ld_addr(Buffer *b, int reg, int ld_off) {
    emit_lea_rbp(b, reg, ld_off);
}

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

/* XMM spill area sits further from rbp than the GP spill area, so XMM
 * spill slots don't collide with GP ones.  gp_spill_area is the total
 * byte-size of the GP spill region (ra_gp->stack_size). */
static int spill_offset_xmm(int slot, int gp_spill_area) {
    return -(gp_spill_area + 8 * (slot + 1));
}

/* Ensure float value `v` is in `dst_xmm`.  Emits movsd or load as needed. */
static void ensure_reg_xmm(Buffer *b, int v, int dst_xmm, const RAResult *ra_xmm,
                           int gp_spill_area) {
    if (!ra_xmm || v < 0 || v >= ra_xmm->num_values) {
        fprintf(stderr, "fakecc: float value %d has no XMM home\n", v);
        exit(1);
    }
    int vr = ra_xmm->reg[v];
    if (vr == dst_xmm) return;
    if (vr >= 0 && vr < 16) {
        emit_sse_mov_rr(b, dst_xmm, vr);
    } else {
        emit_sse_load_spill(b, dst_xmm, spill_offset_xmm(ra_xmm->spill_slot[v],
                                                          gp_spill_area));
    }
}

/* Store to float value v's XMM spill slot if spilled (no-op if in register). */
static void spill_if_needed_xmm(Buffer *b, int v, int src_xmm,
                                const RAResult *ra_xmm, int gp_spill_area) {
    if (!ra_xmm || v < 0 || v >= ra_xmm->num_values) return;
    if (ra_xmm->reg[v] >= 0 && ra_xmm->reg[v] < 16) return;
    emit_sse_store_spill(b, src_xmm,
                         spill_offset_xmm(ra_xmm->spill_slot[v], gp_spill_area));
}

/* Materialize a float/double constant into xmm_dst.  The IEEE-754 bit
 * pattern lives in inst->float_imm (an int64).  Load it via RAX:
 *   movabs rax, <bits>; movq xmm, rax
 * Works for both float (bits in low 32) and double (all 64 bits). */
static void emit_float_const(Buffer *b, int xmm_dst, int64_t bits) {
    /* movabs rax, imm64  →  REX.W B8+0 */
    emit_rex_w(b);
    emit_byte(b, 0xB8);
    for (int i = 0; i < 8; i++)
        emit_byte(b, (uint8_t)(bits >> (i * 8)));
    /* movq xmm_dst, rax */
    emit_movq_xmm_gp(b, xmm_dst, REG_RAX);
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

/* Map an IR comparison opcode to the setcc opcode byte.
 *   signed:   sete/setne/setl/setle/setg/setge  (0x94..)
 *   unsigned: sete/setne/setb/setbe/seta/setae  (0x94/95/92/96/97/93) */
static uint8_t ir_cmp_to_setcc(int ir_op, int is_unsigned) {
    if (!is_unsigned) {
        switch (ir_op) {
        case IR_EQ: return 0x94;
        case IR_NE: return 0x95;
        case IR_LT: return 0x9C;
        case IR_LE: return 0x9E;
        case IR_GT: return 0x9F;
        case IR_GE: return 0x9D;
        default:    return 0x94;
        }
    } else {
        switch (ir_op) {
        case IR_EQ: return 0x94;
        case IR_NE: return 0x95;
        case IR_LT: return 0x92;   /* setb */
        case IR_LE: return 0x96;   /* setbe */
        case IR_GT: return 0x97;   /* seta */
        case IR_GE: return 0x93;   /* setae */
        default:    return 0x94;
        }
    }
}

/* ================================================================== */
/* IR → x86-64 codegen                                                 */
/* ================================================================== */

/* Label patch record — a rel32 to be resolved once we know the label's
 * byte offset inside this function's code. */
typedef struct {
    size_t patch_off;     /* absolute offset of the rel32 field in out->text */
    int label;            /* target label id */
    size_t after_off;     /* offset of the instruction *following* the rel32,
                             used as base for rel32 = target - after */
} Patch;

/* Cross-function call patch: the callee is referenced by name; its code
 * offset only becomes known once all functions have been laid out. */
typedef struct {
    size_t patch_off;
    char  *callee;        /* xstrdup'd */
    size_t after_off;
} CallPatch;

/* Function-address load patch: `lea r, [rip+0]` where the target is a function
 * (e.g. `&add` or a function lvalue).  Resolved against the symbol table. */
typedef struct {
    size_t patch_off;
    char  *fn_name;       /* xstrdup'd function name */
} FnAddrPatch;

/* Compute which callee-saved registers this function actually uses.
 * Sets used_bit for each of REG_RBX / REG_R12 / REG_R13 (0 or 1). */
static void collect_callee_saved(const RAResult *ra, int used[3]) {
    used[0] = used[1] = used[2] = 0;
    if (!ra) return;
    for (int v = 0; v < ra->num_values; v++) {
        int r = ra->reg[v];
        if (r == REG_RBX) used[0] = 1;
        else if (r == REG_R12) used[1] = 1;
        else if (r == REG_R13) used[2] = 1;
    }
}

/* mov [base+off], %reg  →  REX.W 89 [mod=01/10 reg=src rm=base] disp. */
static void emit_store_base_off(Buffer *b, int base, int reg, int off) {
    emit_rex_wrb(b, 1, reg, base);
    emit_byte(b, 0x89);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, reg, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, reg, base);
        emit_int32(b, off);
    }
}

/* mov %reg, [base+off] (load)  →  REX.W 8B [mod=01/10 reg=dst rm=base] disp. */
static void emit_load_base_off(Buffer *b, int dst, int base, int off) {
    emit_rex_wrb(b, 1, dst, base);
    emit_byte(b, 0x8B);
    if (off >= -128 && off <= 127) {
        emit_modrm(b, 1, dst, base);
        emit_byte(b, (uint8_t)(off & 0xFF));
    } else {
        emit_modrm(b, 2, dst, base);
        emit_int32(b, off);
    }
}

/* va_list field offsets in our __va_list_tag layout (members padded to 8). */
#define VA_GP_OFF   0
#define VA_FP_OFF   8
#define VA_OV_OFF   16
#define VA_REG_OFF  24

/* Implement `va_start(ap, last)`: fill the four va_list fields.  The initial
 * offsets (gp_offset, fp_offset, overflow_arg_off) are passed in; they are
 * computed once per function from its named-arg layout.  reg_save_area is the
 * save-area base = rsp (constant after the prologue). */
static void emit_va_start(Buffer *b, const IRInst *inst, const RAResult *ra,
                          int gp_offset, int fp_offset, int overflow_off) {
    int ap = inst->call_args[0];
    ensure_reg(b, ap, REG_RAX, ra);
    int ap_reg = REG_RAX;
    /* gp_offset @0 */
    emit_mov_imm(b, REG_RCX, gp_offset);
    emit_store_base_off(b, ap_reg, REG_RCX, VA_GP_OFF);
    /* fp_offset @8 */
    emit_mov_imm(b, REG_RCX, fp_offset);
    emit_store_base_off(b, ap_reg, REG_RCX, VA_FP_OFF);
    /* overflow_arg_area @16 = rbp + first stack-passed arg offset */
    emit_lea_rbp(b, REG_RCX, overflow_off);
    emit_store_base_off(b, ap_reg, REG_RCX, VA_OV_OFF);
    /* reg_save_area @24 = rsp (save area base, constant) */
    emit_mov_rr(b, REG_RCX, REG_RSP);
    emit_store_base_off(b, ap_reg, REG_RCX, VA_REG_OFF);
}

/* Load a GP value from [base+index] into dst without SIB addressing: computes
 * base+index into a scratch via add, then loads from [scratch+0].  Clobbers
 * R11 and the index reg must not be dst. */
static void emit_load_gp_viabase(Buffer *b, int dst, int base, int index) {
    emit_add_rr(b, base, index); /* base += index */
    emit_load_base_off(b, dst, base, 0);
}

/* Implement `va_arg(ap, T)`.  Walks the register-save area while the per-class
 * offset lasts, then falls back to the overflow (stack) area.  Branches on the
 * requested type class (GP vs FP) using inline-patched forward branches.
 *
 * Layout of the save area (addressed as rsp + byte offset, rsp constant):
 *   [rsp+0..+40]   rdi,rsi,rdx,rcx,r8,r9   (8 bytes each, GP regs)
 *   [rsp+48..+176] xmm0-xmm7               (16 bytes each, FP regs)
 * reg_save_area = rsp is filled by va_start. */
static void emit_va_arg(Buffer *b, const IRInst *inst, const RAResult *ra,
                        const RAResult *ra_xmm, int gp_spill_area) {
    int ap = inst->call_args[0];
    ensure_reg(b, ap, REG_RAX, ra);
    int ap_reg = REG_RAX;
    int dst = inst->dst;
    int is_float = inst->is_float;

    if (!is_float) {
        /* ---------------- GP class ---------------- */
        emit_load_base_off(b, REG_RCX, ap_reg, VA_GP_OFF); /* rcx = gp_offset */
        emit_cmp_imm32(b, REG_RCX, 48);
        size_t jae_ov = emit_jcc_rel32(b, 0x83); /* JAE overflow_path */
        /* register path: load from [reg_save_area + gp_offset] */
        emit_load_base_off(b, REG_R11, ap_reg, VA_REG_OFF); /* r11 = save_area */
        emit_load_gp_viabase(b, REG_RDX, REG_R11, REG_RCX);  /* rdx = *[save+gp] */
        emit_load_base_off(b, REG_RCX, ap_reg, VA_GP_OFF);
        emit_add_imm32(b, REG_RCX, 8);
        emit_store_base_off(b, ap_reg, REG_RCX, VA_GP_OFF);
        size_t jmp_end = emit_jmp_rel32(b);
        /* overflow_path: load from [overflow_arg_area], advance by 8 */
        size_t ov_off = b->len;
        patch_rel32(b, jae_ov, ov_off);
        emit_load_base_off(b, REG_R11, ap_reg, VA_OV_OFF); /* r11 = overflow */
        emit_load_base_off(b, REG_RDX, REG_R11, 0);         /* rdx = *[overflow] */
        emit_add_imm32(b, REG_R11, 8);
        emit_store_base_off(b, ap_reg, REG_R11, VA_OV_OFF);
        /* end: */
        patch_rel32(b, jmp_end, b->len);
        /* move loaded value (RDX) into dst home / spill */
        if (dst >= 0) {
            if (ra && dst < ra->num_values && ra->reg[dst] >= 0
                && ra->reg[dst] < 16) {
                int dr = ra->reg[dst];
                if (dr != REG_RDX) emit_mov_rr(b, dr, REG_RDX);
            } else {
                spill_if_needed(b, dst, REG_RDX, ra);
            }
        }
    } else {
        /* ---------------- FP class ---------------- */
        emit_load_base_off(b, REG_RCX, ap_reg, VA_FP_OFF); /* rcx = fp_offset */
        emit_cmp_imm32(b, REG_RCX, 176);
        size_t jae_ov = emit_jcc_rel32(b, 0x83); /* JAE overflow_path */
        /* register path: load FP from [reg_save_area + fp_offset] */
        emit_load_base_off(b, REG_R11, ap_reg, VA_REG_OFF); /* r11 = save_area */
        emit_add_rr(b, REG_R11, REG_RCX); /* r11 = save_area + fp_offset */
        emit_sse_load_base_off(b, 0, REG_R11, 0); /* xmm0 = *[r11] (movsd) */
        emit_load_base_off(b, REG_RCX, ap_reg, VA_FP_OFF);
        emit_add_imm32(b, REG_RCX, 16);
        emit_store_base_off(b, ap_reg, REG_RCX, VA_FP_OFF);
        size_t jmp_end = emit_jmp_rel32(b);
        /* overflow_path: load FP from [overflow_arg_area], advance by 8 */
        size_t ov_off = b->len;
        patch_rel32(b, jae_ov, ov_off);
        emit_load_base_off(b, REG_R11, ap_reg, VA_OV_OFF); /* r11 = overflow */
        emit_sse_load_base_off(b, 0, REG_R11, 0); /* xmm0 = *[overflow] */
        emit_add_imm32(b, REG_R11, 8);
        emit_store_base_off(b, ap_reg, REG_R11, VA_OV_OFF);
        /* end: */
        patch_rel32(b, jmp_end, b->len);
        if (dst >= 0) {
            spill_if_needed_xmm(b, dst, 0, ra_xmm, gp_spill_area);
        }
    }
}

/* Emit epilogue: restore callee-saved (reverse order), tear down frame, ret. */
static void emit_epilogue(Buffer *b, int stack_size, const int cs_used[3]) {
    emit_add_rsp_imm32(b, stack_size);
    if (cs_used[2]) emit_pop_r(b, REG_R13);
    if (cs_used[1]) emit_pop_r(b, REG_R12);
    if (cs_used[0]) emit_pop_r(b, REG_RBX);
    emit_byte(b, 0x5D);   /* pop %rbp */
    emit_byte(b, 0xC3);   /* ret */
}

void codegen(const IRModule *ir, EmitModule *out) {
    /* Emit globals into the appropriate section (.rodata for read-only,
     * .data for initialized mutable, .bss for zero-initialized) and register
     * each as a defined symbol with its linkage binding. */
    for (size_t gi = 0; gi < ir->globals.len; gi++) {
        const IRGlobal *g = &ir->globals.data[gi];
        uint8_t binding = g->is_static ? 0 /* STB_LOCAL */ : 1 /* STB_GLOBAL */;
        uint16_t shndx;
        size_t off;
        if (g->is_readonly) {
            shndx = SECT_RODATA;
            off = out->rodata.len;
            buffer_append(&out->rodata, g->init_bytes, g->size);
            while (out->rodata.len & 7) { char z = 0; buffer_append(&out->rodata, &z, 1); }
        } else if (g->init_bytes) {
            shndx = SECT_DATA;
            off = out->data.len;
            buffer_append(&out->data, g->init_bytes, g->size);
            while (out->data.len & 7) { char z = 0; buffer_append(&out->data, &z, 1); }
        } else {
            shndx = SECT_BSS;
            off = out->bss_size;
            out->bss_size += g->size;
            while (out->bss_size & 7) out->bss_size++;
        }
        emit_module_add_symbol(out, g->name, binding, 1 /* STT_OBJECT */,
                               shndx, off, g->size);
    }

    /* Cross-function call patches (accumulated across every function). */
    CallPatch *call_patches = NULL;
    size_t num_call_patches = 0, cap_call_patches = 0;
    /* Function-address load patches (`&func` / function lvalue). */
    FnAddrPatch *fnaddr_patches = NULL;
    size_t num_fnaddr_patches = 0, cap_fnaddr_patches = 0;

    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        const RAResult *ra = (const RAResult *)fn->ra;
        const RAResult *ra_xmm = (const RAResult *)fn->ra_xmm;
        size_t start_offset = out->text.len;

        /* ---- Prologue ---- */
        int cs_used[3];
        collect_callee_saved(ra, cs_used);

        /* Compute pinned-alloca frame area: byte-offset from rbp for each
         * IR_ALLOCA with alloca_bytes > 0.  Offsets are placed above the
         * spill areas (further from rbp, i.e. more negative).  The frame
         * layout below rbp is: [GP spills][XMM spills][pinned allocas]. */
        int *alloca_off = xmalloc(fn->next_value_id * sizeof(int));
        for (int i = 0; i < fn->next_value_id; i++) alloca_off[i] = 0;
        int gp_spill_area = ra ? ra->stack_size : 0;
        int xmm_spill_area = ra_xmm ? ra_xmm->stack_size : 0;
        int pinned_area = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            if (inst->op == IR_ALLOCA && inst->alloca_bytes > 0 && inst->dst >= 0) {
                int bytes = inst->alloca_bytes;
                if (bytes % 8 != 0) bytes += 8 - (bytes % 8);
                pinned_area += bytes;
                alloca_off[inst->dst] = -(gp_spill_area + xmm_spill_area + pinned_area);
            }
        }

        /* Long-double (width 16) slot assignment.  ld values never live in
         * registers — each gets a fixed 16-byte slot addressed rbp-relative
         * via ld_off[v].  Locals reuse their pinned-alloca offset; operation
         * results and constants are packed into a dedicated ld area below the
         * pinned area.  ld params are handled in the param-binding loop
         * (positive stack offset).  Unassigned ld values keep ld_off = 0 and
         * are treated as slot 0 (only valid once every ld value is assigned). */
        int *ld_off = xmalloc(fn->next_value_id * sizeof(int));
        for (int i = 0; i < fn->next_value_id; i++) ld_off[i] = 0;
        int ld_area = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            if (!value_is_ld(fn, inst->dst)) continue;
            if (inst->op == IR_ALLOCA && inst->alloca_bytes > 0) {
                /* ld local: reuse its pinned-alloca slot (16-byte aligned). */
                ld_off[inst->dst] = alloca_off[inst->dst];
            } else if (inst->op != IR_PARAM) {
                /* ld operation result or constant: fresh 16-byte slot. */
                ld_area += 16;
                ld_off[inst->dst] = -(gp_spill_area + xmm_spill_area + pinned_area + ld_area);
            }
        }

        int stack_size = gp_spill_area + xmm_spill_area + pinned_area + ld_area;
        if (!ra && stack_size == 0) stack_size = 8 * fn->next_value_id;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);

        /* Variadic: reserve a 176-byte register-save area at the bottom of the
         * frame (below the spills/allocas).  48 bytes for rdi,rsi,rdx,rcx,r8,r9
         * then 128 bytes for xmm0-xmm7.  rsp is constant after the prologue, so
         * [rsp+off] addresses it stably for the whole function.  The save area
         * must be 16-byte aligned (movaps stores xmm regs), so the frame bottom
         * (rsp) must be 16-aligned. */
        if (fn->is_variadic) stack_size += 176;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);

        /* Alignment invariant: on entry rsp % 16 == 8 (call pushed ret).
         * After "push rbp" rsp % 16 == 0.  Each callee-saved push adds 8;
         * if odd count, we need one extra bump so that rsp (the save-area base)
         * ends up 16-aligned for the movaps stores. */
        int cs_count = cs_used[0] + cs_used[1] + cs_used[2];
        if (((cs_count) & 1) != 0) stack_size += 8;

        emit_byte(&out->text, 0x55);              /* pushq %rbp */
        emit_rex_w(&out->text);
        emit_byte(&out->text, 0x89);
        emit_byte(&out->text, 0xE5);              /* movq %rsp, %rbp */

        if (cs_used[0]) emit_push_r(&out->text, REG_RBX);
        if (cs_used[1]) emit_push_r(&out->text, REG_R12);
        if (cs_used[2]) emit_push_r(&out->text, REG_R13);

        /* Materialize incoming SysV arg registers into their allocated
         * homes.  SysV AMD64 passes the first 6 INTEGER-class args in
         * RDI,RSI,RDX,RCX,R8,R9 and the first 8 SSE-class (float/double)
         * args in XMM0-XMM7, with the two counters independent.  Args that
         * don't fit in their class's registers are passed on the stack at
         * [rbp + 16 + 8*k].
         *
         * We first decide, per param, how it arrives (GP reg / XMM reg /
         * stack).  Then a push-then-pop dance (pushing every used arg
         * register, GP via `push`, XMM via `sub rsp,8; movsd [rsp],xmm`)
         * in reverse param order guarantees no clobber can lose data even
         * when a param's home is another param's source register. */
        /* Per-param arrival info.  arrive_reg = native register code, or -1
         * for stack-passed.  arrive_is_xmm tells which file arrive_reg is
         * in.  stack_off is the [rbp+..] offset for stack-passed args.
         * PARAMs are contiguous at function start (see ir.c lowering). */
        int nparams = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            if (fn->insts.data[j].op == IR_PARAM) nparams++;
            else break;
        }
        int arrive_reg[64];
        int arrive_is_xmm[64];
        int stack_off[64];
        int gp_reg_idx = 0, xmm_reg_idx = 0, stack_arg_idx = 0;
        for (int p = 0; p < nparams; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int is_ld = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                         value_is_ld(fn, pi->dst));
            int is_float = !is_ld && (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                            value_is_float_class(fn, pi->dst));
            if (is_ld) {
                /* long double: SysV passes in a 16-byte stack slot, NOT XMM.
                 * Record its [rbp+..] offset; codegen loads it via fldt there. */
                arrive_reg[p] = -1;
                arrive_is_xmm[p] = 0;
                stack_off[p] = 16 + 8 * stack_arg_idx;
                stack_arg_idx += 2; /* 16-byte slot = two 8-byte stack slots */
                ld_off[pi->dst] = stack_off[p]; /* positive rbp-relative offset */
            } else if (is_float) {
                if (xmm_reg_idx < 8) {
                    arrive_reg[p] = xmm_reg_idx; /* native XMM code 0..7 */
                    arrive_is_xmm[p] = 1;
                    xmm_reg_idx++;
                } else {
                    arrive_reg[p] = -1;
                    arrive_is_xmm[p] = 0;
                    stack_off[p] = 16 + 8 * stack_arg_idx++;
                }
            } else {
                if (gp_reg_idx < 6) {
                    arrive_reg[p] = SYSV_ARG_REGS[gp_reg_idx++];
                    arrive_is_xmm[p] = 0;
                } else {
                    arrive_reg[p] = -1;
                    arrive_is_xmm[p] = 0;
                    stack_off[p] = 16 + 8 * stack_arg_idx++;
                }
            }
        }
        /* Variadic: compute the initial va_list field values.  The named args
         * consume the first gp_reg_idx GP and xmm_reg_idx FP register slots, so
         * the first variadic arg begins at those offsets in the save area.  The
         * overflow (stack) area starts after the named stack-passed args. */
        int va_gp_offset = 0, va_fp_offset = 0, va_overflow_off = 16;
        if (fn->is_variadic) {
            va_gp_offset = 8 * gp_reg_idx;
            va_fp_offset = 48 + 16 * xmm_reg_idx;
            va_overflow_off = 16 + 8 * stack_arg_idx;
        }

        /* Allocate the frame.  For variadic functions this also reserves the
         * 176-byte register-save area at the bottom. */
        emit_sub_rsp_imm32(&out->text, stack_size); /* sub $N, %rsp */

        /* Variadic: save the incoming argument registers into the save area at
         * the bottom of the frame BEFORE the param dance clobbers them.  The
         * dance (below) pushes/pops arg regs to move them into their homes,
         * which would destroy the values we need for the save area.  rsp is
         * constant from here on, so [rsp+off] addresses the save area stably. */
        if (fn->is_variadic) {
            /* GP regs rdi,rsi,rdx,rcx,r8,r9 → [rsp+0..+40]. */
            emit_store_rsp_off(&out->text, REG_RDI, 0);
            emit_store_rsp_off(&out->text, REG_RSI, 8);
            emit_store_rsp_off(&out->text, REG_RDX, 16);
            emit_store_rsp_off(&out->text, REG_RCX, 24);
            emit_store_rsp_off(&out->text, REG_R8, 32);
            emit_store_rsp_off(&out->text, REG_R9, 40);
            /* FP regs xmm0-xmm7 → [rsp+48..+176] via movaps (16-byte slots). */
            emit_sse_store_rsp_off(&out->text, 0, 48);
            emit_sse_store_rsp_off(&out->text, 1, 64);
            emit_sse_store_rsp_off(&out->text, 2, 80);
            emit_sse_store_rsp_off(&out->text, 3, 96);
            emit_sse_store_rsp_off(&out->text, 4, 112);
            emit_sse_store_rsp_off(&out->text, 5, 128);
            emit_sse_store_rsp_off(&out->text, 6, 144);
            emit_sse_store_rsp_off(&out->text, 7, 160);
        }

        /* Push every used arg register in REVERSE param order. */
        for (int p = nparams - 1; p >= 0; p--) {
            if (arrive_reg[p] < 0) continue;
            if (arrive_is_xmm[p]) {
                emit_sub_rsp_imm32(&out->text, 8);
                emit_sse_store_rsp(&out->text, arrive_reg[p]);
            } else {
                emit_push_r(&out->text, arrive_reg[p]);
            }
        }
        /* Pop into each param's allocated home (or spill slot) in order. */
        for (int p = 0; p < nparams; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int is_ld = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                         value_is_ld(fn, pi->dst));
            int is_float = (pi->dst >= 0 && pi->dst < fn->next_value_id &&
                            value_is_float_class(fn, pi->dst));
            if (arrive_reg[p] < 0) {
                /* Stack-passed arg: load from [rbp + stack_off] into home. */
                if (is_ld) {
                    /* long double: already in its 16-byte stack slot (ld_off set
                     * in the param loop); it lives in memory, not XMM — nothing
                     * to load. */
                    continue;
                }
                if (is_float) {
                    int pdr_xmm = (ra_xmm && pi->dst >= 0 &&
                                   pi->dst < ra_xmm->num_values)
                                  ? ra_xmm->reg[pi->dst] : -1;
                    if (pdr_xmm >= 0) {
                        emit_sse_load_spill(&out->text, pdr_xmm, stack_off[p]);
                    } else {
                        emit_sse_load_spill(&out->text, XMM_SCRATCH0, stack_off[p]);
                        spill_if_needed_xmm(&out->text, pi->dst, XMM_SCRATCH0,
                                             ra_xmm, gp_spill_area);
                    }
                } else {
                    int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                              ? ra->reg[pi->dst] : -1;
                    if (pdr >= 0) {
                        emit_load_spill(&out->text, pdr, stack_off[p]);
                    } else {
                        emit_load_spill(&out->text, REG_RAX, stack_off[p]);
                        spill_if_needed(&out->text, pi->dst, REG_RAX, ra);
                    }
                }
                continue;
            }
            if (is_float) {
                int pdr_xmm = (ra_xmm && pi->dst >= 0 &&
                               pi->dst < ra_xmm->num_values)
                              ? ra_xmm->reg[pi->dst] : -1;
                if (pdr_xmm >= 0) {
                    emit_sse_load_rsp(&out->text, pdr_xmm);
                } else {
                    emit_sse_load_rsp(&out->text, XMM_SCRATCH0);
                    spill_if_needed_xmm(&out->text, pi->dst, XMM_SCRATCH0,
                                         ra_xmm, gp_spill_area);
                }
                emit_add_rsp_imm32(&out->text, 8);
            } else {
                int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                          ? ra->reg[pi->dst] : -1;
                if (pdr >= 0) {
                    emit_pop_r(&out->text, pdr);
                } else {
                    emit_pop_r(&out->text, REG_RAX);
                    spill_if_needed(&out->text, pi->dst, REG_RAX, ra);
                }
            }
        }

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
            /* Pick the register file by class: float values live in XMM,
             * everything else in the GP file.  `dr` is the destination's home
             * register (GP code or XMM code) or -1 if spilled. */
            int dr;
            if (value_is_float_class(fn, inst->dst)) {
                dr = (ra_xmm && inst->dst >= 0 && inst->dst < ra_xmm->num_values)
                     ? ra_xmm->reg[inst->dst] : -1;
            } else {
                dr = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                     ? ra->reg[inst->dst] : -1;
            }

            switch (inst->op) {

            case IR_CONST: {
                if (value_is_ld(fn, inst->dst)) {
                    /* long double constant: 80-bit value lives in a rodata
                     * global (name in inst->call_name).  lea rcx,[rip+0] with a
                     * PC32 reloc targeting the global symbol, fldt [rcx],
                     * fstpt into dst's slot.  Clobbers RCX. */
                    size_t patch = emit_lea_rip(&out->text, REG_RCX);
                    int gsym = emit_module_find_symbol(out, inst->call_name);
                    if (gsym < 0)
                        gsym = emit_module_add_undefined(out, inst->call_name);
                    emit_module_add_reloc(out, patch, R_X86_64_PC32, gsym, -4);
                    emit_x87_fldtRCX(&out->text); /* fldt [rcx] */
                    emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (inst->is_float) {
                    /* Float constant: bit pattern in float_imm.  Materialize via
                     * RAX (movabs) then movq into the XMM file. */
                    if (dr >= 0) {
                        emit_float_const(&out->text, dr, inst->float_imm);
                    } else {
                        emit_float_const(&out->text, XMM_SCRATCH0, inst->float_imm);
                        spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                            ra_xmm, gp_spill_area);
                    }
                } else if (dr >= 0) {
                    emit_mov_imm(&out->text, dr, inst->imm);
                } else {
                    emit_mov_imm(&out->text, REG_RAX, inst->imm);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_ADD: {
                /* Stage both operands into scratch (rax = a, rcx = b) so that
                 * loading `a` into `dr` can't clobber `b` if reg[b] == dr. */
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_add_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_SUB: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_sub_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_MUL: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                emit_imul_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_DIV:
            case IR_MOD: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                if (inst->is_unsigned) {
                    /* Unsigned: zero-extend rax into rdx then div. */
                    emit_xor_rr(&out->text, REG_RDX);
                    /* div %rcx: F7 /6 */
                    emit_rex_w(&out->text);
                    emit_byte(&out->text, 0xF7);
                    emit_byte(&out->text, 0xF1);
                } else {
                    emit_cqto(&out->text);
                    emit_idiv_rcx(&out->text);
                }
                if (inst->op == IR_DIV) {
                    mask_to_width(&out->text, REG_RAX, inst->width);
                    if (dr >= 0 && dr != REG_RAX)
                        emit_mov_rr(&out->text, dr, REG_RAX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                } else {
                    mask_to_width(&out->text, REG_RDX, inst->width);
                    if (dr >= 0)
                        emit_mov_rr(&out->text, dr, REG_RDX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                }
                break;
            }

            case IR_NEG: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_neg_r(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_BNOT: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_not_r(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_BAND:
            case IR_BOR:
            case IR_BXOR: {
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                if (inst->op == IR_BAND)
                    emit_and_rr(&out->text, REG_RAX, REG_RCX);
                else if (inst->op == IR_BOR)
                    emit_or_rr(&out->text, REG_RAX, REG_RCX);
                else
                    emit_bitxor_rr(&out->text, REG_RAX, REG_RCX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_SHL:
            case IR_SHR: {
                /* Shift count must be in cl. Ensure b into rcx FIRST (before
                 * a into rax) only if a isn't bound to rcx — but ensure_reg
                 * for a uses rax, so order is safe: a→rax, b→rcx. */
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (inst->op == IR_SHL)
                    emit_shl_rcx(&out->text, REG_RAX);
                else if (inst->is_unsigned)
                    emit_shr_rcx(&out->text, REG_RAX);
                else
                    emit_sar_rcx(&out->text, REG_RAX);
                mask_to_width(&out->text, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
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
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                ensure_reg(&out->text, inst->b, REG_RCX, ra);
                uint8_t cc = ir_cmp_to_setcc(inst->op, inst->is_unsigned);
                emit_cmp_produce(&out->text, REG_RAX, REG_RCX, REG_RDX, cc);
                if (dr >= 0) {
                    if (dr != REG_RDX) emit_mov_rr(&out->text, dr, REG_RDX);
                } else {
                    spill_if_needed(&out->text, inst->dst, REG_RDX, ra);
                }
                break;
            }

            case IR_ALLOCA:
                break;

            case IR_COPY: {
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
                    break;
                }
                if (dr >= 0) {
                    ensure_reg(&out->text, inst->a, dr, ra);
                } else {
                    old_load(&out->text, inst->a, REG_RAX);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_SEXT:
            case IR_ZEXT: {
                /* imm = source width */
                int src_w = inst->imm;
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                if (inst->op == IR_SEXT)
                    emit_movsx_rr(&out->text, REG_RAX, REG_RAX, src_w);
                else
                    emit_movzx_rr(&out->text, REG_RAX, REG_RAX, src_w);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_TRUNC: {
                /* No-op at register level: SSA values live in 64-bit regs;
                 * narrower uses (stores, cmp width) will mask via width field.
                 * Copy value if regs differ. */
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
                    break;
                }
                if (dr >= 0) {
                    ensure_reg(&out->text, inst->a, dr, ra);
                } else {
                    old_load(&out->text, inst->a, REG_RAX);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_LOAD: {
                /* Rarely reached: mem2reg usually eliminates LOAD.  If a LOAD
                 * survives, treat it as a copy of the alloca's spill slot
                 * with narrow-width sign/zero-extend semantics.  We reuse
                 * the same copy fast-path since spill layout is 8B/slot. */
                if (dr >= 0) {
                    ensure_reg(&out->text, inst->a, dr, ra);
                } else {
                    old_load(&out->text, inst->a, REG_RAX);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_STORE: {
                int sr = (ra && inst->b >= 0 && inst->b < ra->num_values)
                         ? ra->reg[inst->b] : -1;
                if (sr >= 0) {
                    spill_if_needed(&out->text, inst->a, sr, ra);
                } else {
                    /* Fallback (no ra or spilled): load b's slot into rax,
                     * then store rax to a's slot. */
                    old_load(&out->text, inst->b, REG_RAX);
                    old_store(&out->text, inst->a, REG_RAX);
                }
                break;
            }

            case IR_ADDR: {
                /* dst = &alloca_slot.  Get alloca's byte-offset from rbp. */
                int off = 0;
                if (inst->a >= 0 && inst->a < fn->next_value_id)
                    off = alloca_off[inst->a];
                if (off == 0) {
                    /* Alloca wasn't laid out as pinned — this is a bug in IR-gen
                     * (should have set alloca_bytes>0). */
                    fprintf(stderr, "fakecc: IR_ADDR on non-pinned alloca %d\n", inst->a);
                    exit(1);
                }
                if (dr >= 0) {
                    emit_lea_rbp(&out->text, dr, off);
                } else {
                    emit_lea_rbp(&out->text, REG_RAX, off);
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_GADDR: {
                /* dst = &global; target name in inst->call_name.  Emit
                 * `lea r, [rip+0]` and record a PC32 relocation targeting the
                 * global's symbol (defined in this module or external). */
                int target = dr >= 0 ? dr : REG_RAX;
                size_t patch = emit_lea_rip(&out->text, target);
                int gsym = emit_module_find_symbol(out, inst->call_name);
                if (gsym < 0)
                    gsym = emit_module_add_undefined(out, inst->call_name);
                emit_module_add_reloc(out, patch, R_X86_64_PC32, gsym, -4);
                if (dr < 0)
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                break;
            }

            case IR_FADDR: {
                /* dst = &function; function name in inst->call_name.  Emit
                 * `lea r, [rip+0]` and record an FnAddrPatch resolved against
                 * the code symbol table (functions live in .code, not .data). */
                int target = dr >= 0 ? dr : REG_RAX;
                size_t patch = emit_lea_rip(&out->text, target);
                if (num_fnaddr_patches >= cap_fnaddr_patches) {
                    cap_fnaddr_patches = cap_fnaddr_patches ? cap_fnaddr_patches * 2 : 8;
                    fnaddr_patches = xrealloc(fnaddr_patches,
                                               cap_fnaddr_patches * sizeof(FnAddrPatch));
                }
                fnaddr_patches[num_fnaddr_patches].patch_off = patch;
                fnaddr_patches[num_fnaddr_patches].fn_name = xstrdup(inst->call_name);
                num_fnaddr_patches++;
                if (dr < 0)
                    spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                break;
            }

            case IR_LOAD_PTR: {
                /* dst = *ptr.  ptr = inst->a. */
                ensure_reg(&out->text, inst->a, REG_RCX, ra);
                if (value_is_ld(fn, inst->dst)) {
                    /* long double load: fldt [ptr] → st0; fstpt to dst's slot.
                     * ld values are slot-backed (no XMM/GP register). */
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 5, REG_RCX & 7); /* fldt [rcx] */
                    emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (value_is_float_class(fn, inst->dst)) {
                    /* Float load: movsd/movss xmm, [ptr]. */
                    int is_float = (inst->width == 4);
                    emit_sse_load_via_ptr(&out->text,
                                          dr >= 0 ? dr : XMM_SCRATCH0,
                                          REG_RCX, is_float);
                    if (dr < 0)
                        spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                            ra_xmm, gp_spill_area);
                } else {
                    emit_load_via_ptr(&out->text,
                                      dr >= 0 ? dr : REG_RAX,
                                      REG_RCX, inst->width, inst->is_unsigned);
                    if (dr < 0)
                        spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_STORE_PTR: {
                /* *ptr = val.  ptr = inst->a, val = inst->b. */
                ensure_reg(&out->text, inst->a, REG_RCX, ra);
                if (value_is_ld(fn, inst->b)) {
                    /* long double store: fldt val's slot → st0; fstpt [ptr].
                     * emit_ld_load clobbers RCX, so reload the pointer into it
                     * after the value is in st0. */
                    emit_ld_load(&out->text, inst->b, ld_off); /* clobbers RCX */
                    ensure_reg(&out->text, inst->a, REG_RCX, ra); /* ptr → RCX */
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 7, REG_RCX & 7); /* fstpt [rcx] */
                } else if (value_is_float_class(fn, inst->b)) {
                    /* Float store: movsd/movss [ptr], xmm. */
                    int is_float = (inst->width == 4);
                    ensure_reg_xmm(&out->text, inst->b, XMM_SCRATCH0, ra_xmm,
                                   gp_spill_area);
                    emit_sse_store_via_ptr(&out->text, REG_RCX, XMM_SCRATCH0,
                                           is_float);
                } else {
                    ensure_reg(&out->text, inst->b, REG_RAX, ra);
                    emit_store_via_ptr(&out->text, REG_RCX, REG_RAX, inst->width);
                }
                break;
            }

            case IR_LABEL: {
                if (inst->imm >= 0 && inst->imm < nlabels) {
                    label_off[inst->imm] = out->text.len;
                }
                break;
            }

            case IR_BR: {
                size_t patch = emit_jmp_rel32(&out->text);
                size_t after = out->text.len;
                ADD_PATCH(patch, inst->imm, after);
                break;
            }

            case IR_CBR: {
                /* CBR: a = cond, imm = true_label, b = false_label.
                 * Emit "test cond,cond; jne true_label; jmp false_label". */
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                emit_test_rr(&out->text, REG_RAX);
                /* jne true_label (0x85) */
                size_t p1 = emit_jcc_rel32(&out->text, 0x85);
                size_t a1 = out->text.len;
                ADD_PATCH(p1, inst->imm, a1);
                /* jmp false_label */
                size_t p2 = emit_jmp_rel32(&out->text);
                size_t a2 = out->text.len;
                ADD_PATCH(p2, inst->b, a2);
                break;
            }

            case IR_PARAM:
                /* Already handled during prologue (push-then-pop dance).
                 * Skip here.  */
                break;

            case IR_CALL: {
                /* __syscall(num, a0..a5) — emit a raw `syscall` instruction.
                 * Linux x86-64 syscall ABI: rax = num, args in rdi/rsi/rdx/r10/r8/r9.
                 * We use the same push-then-pop dance to load args safely. */
                if (inst->call_name && strcmp(inst->call_name, "__syscall") == 0) {
                    static const int SYS_ARG_REGS[7] = {
                        REG_RAX, REG_RDI, REG_RSI, REG_RDX,
                        REG_R10, REG_R8, REG_R9
                    };
                    int nargs = inst->call_nargs;
                    /* Push args in reverse order */
                    for (int k = 0; k < nargs; k++) {
                        ensure_reg(&out->text, inst->call_args[k], REG_RCX, ra);
                        emit_push_r(&out->text, REG_RCX);
                    }
                    /* Pop into syscall arg regs in reverse (arg 0 = num → RAX) */
                    for (int k = nargs - 1; k >= 0; k--) {
                        emit_pop_r(&out->text, SYS_ARG_REGS[k]);
                    }
                    /* Emit `syscall` — 0F 05 */
                    emit_byte(&out->text, 0x0F);
                    emit_byte(&out->text, 0x05);
                    /* Result in RAX; move to dst or spill. */
                    if (dr >= 0) {
                        if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                    } else {
                        spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                    }
                    break;
                }
                /* va_start / va_arg / va_end — variadic builtins.  They are named
                 * calls (call_name set) and operate on the va_list pointer that
                 * was passed as call_args[0].  Dispatch by name. */
                if (inst->call_name && strcmp(inst->call_name, "va_start") == 0) {
                    emit_va_start(&out->text, inst, ra, va_gp_offset,
                                  va_fp_offset, va_overflow_off);
                    break;
                }
                if (inst->call_name && strcmp(inst->call_name, "va_end") == 0) {
                    /* va_end is a no-op for our ABI. */
                    break;
                }
                if (inst->call_name && strcmp(inst->call_name, "va_arg") == 0) {
                    emit_va_arg(&out->text, inst, ra, ra_xmm, gp_spill_area);
                    break;
                }
                int nargs = inst->call_nargs;
                /* SysV AMD64 assigns the first 6 INTEGER-class args to
                 * RDI,RSI,RDX,RCX,R8,R9 and the first 8 SSE-class (float/
                 * double) args to XMM0-XMM7, the two counters independent.
                 * Args that don't fit in their class's registers are passed
                 * on the stack at [rsp+8], [rsp+16], ...  Decide each arg's
                 * target up front. */
                int target_reg[IR_CALL_MAX_ARGS];   /* native reg code, -1=stack */
                int target_is_xmm[IR_CALL_MAX_ARGS];
                int n_gp = 0, n_xmm = 0, n_stack = 0;
                for (int k = 0; k < nargs; k++) {
                    int is_ld = value_is_ld(fn, inst->call_args[k]);
                    int is_float = !is_ld && value_is_float_class(fn, inst->call_args[k]);
                    if (is_ld) {
                        /* long double arg: passed on the stack in a 16-byte slot
                         * (two 8-byte positions), NOT in XMM. */
                        target_reg[k] = -1;
                        target_is_xmm[k] = 0;
                        n_stack += 2;
                    } else if (is_float) {
                        if (n_xmm < 8) {
                            target_reg[k] = n_xmm; /* XMM0..7 */
                            target_is_xmm[k] = 1;
                            n_xmm++;
                        } else {
                            target_reg[k] = -1;
                            target_is_xmm[k] = 0;
                            n_stack++;
                        }
                    } else {
                        if (n_gp < 6) {
                            target_reg[k] = SYSV_ARG_REGS[n_gp++];
                            target_is_xmm[k] = 0;
                        } else {
                            target_reg[k] = -1;
                            target_is_xmm[k] = 0;
                            n_stack++;
                        }
                    }
                }
                /* Alignment: at call time rsp must be 16-aligned.  The reg-arg
                 * dance is balanced (pushes == pops), so only the nstack
                 * pushes shift alignment.  Pad iff nstack is odd. */
                int need_pad = (n_stack & 1);
                if (need_pad) emit_sub_rsp_imm32(&out->text, 8);

                /* For indirect calls, load the callee into R11 BEFORE the arg
                 * dance so it survives the push/pop clobbers.  R11 is
                 * caller-saved and not a SysV arg reg. */
                if (!inst->call_name)
                    ensure_reg(&out->text, inst->call_callee, REG_R11, ra);

                /* Push stack-passed args right-to-left (highest index first)
                 * so they end up at [rsp+8], [rsp+16], ... in order. */
                for (int k = nargs - 1; k >= 0; k--) {
                    if (target_reg[k] >= 0) continue; /* reg-passed */
                    if (value_is_ld(fn, inst->call_args[k])) {
                        /* long double: 16-byte stack slot.  fldt its home slot,
                         * alloc 16 bytes, fstpt [rsp].  Clobbers RCX. */
                        emit_ld_load(&out->text, inst->call_args[k], ld_off);
                        emit_sub_rsp_imm32(&out->text, 16);
                        emit_x87_fstpt_rsp(&out->text);
                    } else if (target_is_xmm[k]) {
                        ensure_reg_xmm(&out->text, inst->call_args[k],
                                       XMM_SCRATCH0, ra_xmm, gp_spill_area);
                        emit_sub_rsp_imm32(&out->text, 8);
                        emit_sse_store_rsp(&out->text, XMM_SCRATCH0);
                    } else {
                        ensure_reg(&out->text, inst->call_args[k], REG_RAX, ra);
                        emit_push_r(&out->text, REG_RAX);
                    }
                }

                /* Reg-arg dance: push all reg-arg values in reverse order,
                 * then load them into their targets in forward order.  Saving
                 * everything to the stack first dodges cross-arg clobbers. */
                for (int k = nargs - 1; k >= 0; k--) {
                    if (target_reg[k] < 0) continue; /* stack-passed */
                    if (target_is_xmm[k]) {
                        ensure_reg_xmm(&out->text, inst->call_args[k],
                                       XMM_SCRATCH0, ra_xmm, gp_spill_area);
                        emit_sub_rsp_imm32(&out->text, 8);
                        emit_sse_store_rsp(&out->text, XMM_SCRATCH0);
                    } else {
                        ensure_reg(&out->text, inst->call_args[k], REG_RAX, ra);
                        emit_push_r(&out->text, REG_RAX);
                    }
                }
                /* Distribute in forward order (arg 0 on top of the dance). */
                for (int k = 0; k < nargs; k++) {
                    if (target_reg[k] < 0) continue; /* stack-passed */
                    if (target_is_xmm[k]) {
                        emit_sse_load_rsp(&out->text, target_reg[k]);
                        emit_add_rsp_imm32(&out->text, 8);
                    } else {
                        emit_pop_r(&out->text, target_reg[k]);
                    }
                }
                /* AL = number of vector registers used (ABI requirement for
                 * variadic callees; harmless otherwise). */
                emit_byte(&out->text, 0xB0 | REG_RAX); /* mov al, imm8 */
                emit_byte(&out->text, (uint8_t)n_xmm);

                if (inst->call_name) {
                    /* Direct named call: emit call rel32 with a cross-function patch. */
                    size_t poff = emit_call_rel32(&out->text);
                    size_t aft = out->text.len;
                    if (num_call_patches >= cap_call_patches) {
                        cap_call_patches = cap_call_patches ? cap_call_patches * 2 : 8;
                        call_patches = xrealloc(call_patches,
                                                 cap_call_patches * sizeof(CallPatch));
                    }
                    call_patches[num_call_patches].patch_off = poff;
                    call_patches[num_call_patches].callee = xstrdup(inst->call_name);
                    call_patches[num_call_patches].after_off = aft;
                    num_call_patches++;
                } else {
                    /* Indirect call: the callee is already in R11.  Emit
                     * `call *%r11` (FF /2). */
                    emit_indirect_call(&out->text, REG_R11);
                }

                /* Tear down stack args + padding. */
                int cleanup = n_stack * 8 + (need_pad ? 8 : 0);
                if (cleanup > 0) emit_add_rsp_imm32(&out->text, cleanup);

                if (value_is_ld(fn, inst->dst)) {
                    /* long double result comes back in st0 (SysV ld ABI).  fstpt
                     * it into dst's slot.  Clobbers RCX.  Void: dst == -1. */
                    if (inst->dst >= 0)
                        emit_ld_store(&out->text, inst->dst, ld_off);
                } else if (value_is_float_class(fn, inst->dst)) {
                    /* Float result comes back in XMM0 (SysV float ABI).  Move
                     * it to dst's XMM home, or spill.  Void: dst == -1. */
                    if (inst->dst >= 0) {
                        if (dr >= 0 && dr != 0)
                            emit_sse_mov_rr(&out->text, dr, 0); /* 0 == XMM0 */
                        else if (dr < 0)
                            spill_if_needed_xmm(&out->text, inst->dst, 0,
                                                ra_xmm, gp_spill_area);
                    }
                } else {
                    /* Result is in RAX; move to dst home (or spill).  A void
                     * call has dst == -1 (no result) — leave RAX alone. */
                    if (inst->dst >= 0) {
                        if (dr >= 0) {
                            if (dr != REG_RAX) emit_mov_rr(&out->text, dr, REG_RAX);
                        } else {
                            spill_if_needed(&out->text, inst->dst, REG_RAX, ra);
                        }
                    }
                }
                break;
            }

            case IR_RETURN: {
                /* Void function: bare `return;` — no value in a. */
                if (inst->a != -1) {
                    if (value_is_ld(fn, inst->a))
                        emit_ld_load(&out->text, inst->a, ld_off); /* → st0 */
                    else if (value_is_float_class(fn, inst->a))
                        ensure_reg_xmm(&out->text, inst->a, 0, ra_xmm,
                                       gp_spill_area); /* XMM0 */
                    else
                        ensure_reg(&out->text, inst->a, REG_RAX, ra);
                }
                emit_epilogue(&out->text, stack_size, cs_used);
                break;
            }

            case IR_FADD:
            case IR_FSUB:
            case IR_FMUL:
            case IR_FDIV: {
                if (value_is_ld(fn, inst->dst)) {
                    /* long double arithmetic: ld a, ld b, f**p, fstpt dst.
                     * x87 stack ends empty (the `*p` pop + fstpt clear st0). */
                    emit_ld_load(&out->text, inst->a, ld_off);
                    emit_ld_load(&out->text, inst->b, ld_off);
                    int op;
                    switch (inst->op) {
                    /* load a then b (st0=b,st1=a); the pop-op does st1 = st1 op st0
                     * (faddp/fmulp) or st1 = st0 op st1 (fsubrp/fdivp) — chosen
                     * so the result is a+b, a-b, a*b, a/b respectively. */
                    case IR_FADD: op = 0xC1; break; /* DE C1: faddp */
                    case IR_FSUB: op = 0xE9; break; /* DE E9: fsubrp */
                    case IR_FMUL: op = 0xC9; break; /* DE C9: fmulp */
                    default:      op = 0xF9; break; /* DE F9: fdivp */
                    }
                    emit_x87_arith_pop(&out->text, op);
                    emit_ld_store(&out->text, inst->dst, ld_off);
                    break;
                }
                /* dst = a op b, all float.  Stage both operands into XMM
                 * scratch regs (never-allocated scratch0/1), compute into
                 * scratch0, then move to dst home or spill. */
                int is_float = (inst->width == 4);
                ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                               gp_spill_area);
                ensure_reg_xmm(&out->text, inst->b, XMM_SCRATCH1, ra_xmm,
                               gp_spill_area);
                int op;
                switch (inst->op) {
                case IR_FADD: op = 0x58; break;
                case IR_FSUB: op = 0x5C; break;
                case IR_FMUL: op = 0x59; break;
                default:      op = 0x5E; break; /* FDIV */
                }
                emit_sse_arith(&out->text, op, XMM_SCRATCH0, XMM_SCRATCH1,
                               is_float);
                if (dr >= 0 && dr != XMM_SCRATCH0)
                    emit_sse_mov_rr(&out->text, dr, XMM_SCRATCH0);
                spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                    ra_xmm, gp_spill_area);
                break;
            }

            case IR_FCMP: {
                if (value_is_ld(fn, inst->a) || value_is_ld(fn, inst->b)) {
                    /* long double comparison — matches GCC's fcomip sequence.
                     * fcomip computes st0-st1, sets flags, pops st0.  We always
                     * read with seta/setae/sete/setne and pick direction via
                     * load order (LT/LE load a first; GT/GE load b first): see
                     * GCC cmplt/cmpgt.  Zero RDX before (xor clobbers flags). */
                    int is_lt_le = (inst->is_unsigned == 0 || inst->is_unsigned == 1);
                    emit_xor_rr(&out->text, REG_RDX);
                    if (is_lt_le) {
                        emit_ld_load(&out->text, inst->a, ld_off); /* st0=a */
                        emit_ld_load(&out->text, inst->b, ld_off); /* st0=b,st1=a */
                    } else {
                        emit_ld_load(&out->text, inst->b, ld_off); /* st0=b */
                        emit_ld_load(&out->text, inst->a, ld_off); /* st0=a,st1=b */
                    }
                    emit_x87_fcomip(&out->text);
                    emit_byte(&out->text, 0xDD); emit_byte(&out->text, 0xD8); /* fstp %st(0) */
                    uint8_t cc;
                    switch (inst->is_unsigned) {
                    case 0: cc = 0x97; break; /* LT: a<b ↔ b>a → seta */
                    case 1: cc = 0x93; break; /* LE: a<=b ↔ b>=a → setae */
                    case 2: cc = 0x97; break; /* GT: a>b → seta */
                    case 3: cc = 0x93; break; /* GE: a>=b → setae */
                    case 4: cc = 0x94; break; /* EQ → sete */
                    default: cc = 0x95; break; /* NE → setne */
                    }
                    emit_setcc_r(&out->text, cc, REG_RDX);
                    int dr_gp = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                                ? ra->reg[inst->dst] : -1;
                    if (dr_gp >= 0 && dr_gp != REG_RDX)
                        emit_mov_rr(&out->text, dr_gp, REG_RDX);
                    spill_if_needed(&out->text, inst->dst,
                                    dr_gp >= 0 ? dr_gp : REG_RDX, ra);
                    break;
                }
                /* dst (int) = (a op b) ? 1 : 0, a/b float.  The comparison is
                 * encoded in inst->is_unsigned: 0=LT 1=LE 2=GT 3=GE 4=EQ
                 * 5=NE.  ucomi sets flags; setcc produces the 0/1 result. */
                int is_float = (inst->width == 4);
                ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                               gp_spill_area);
                ensure_reg_xmm(&out->text, inst->b, XMM_SCRATCH1, ra_xmm,
                               gp_spill_area);
                /* Map the FCMP ordering to the matching setcc byte. */
                uint8_t cc;
                switch (inst->is_unsigned) {
                case 0: cc = 0x92; break; /* LT → setb */
                case 1: cc = 0x96; break; /* LE → setbe */
                case 2: cc = 0x97; break; /* GT → seta */
                case 3: cc = 0x93; break; /* GE → setae */
                case 4: cc = 0x94; break; /* EQ → sete */
                default: cc = 0x95; break; /* NE → setne */
                }
                /* setcc needs RDX (a GP scratch not used by the SSE staging).
                 * Result is int (width 4); dr is a GP register.  Zero RDX BEFORE
                 * the compare: ucomi sets the flags, and setcc reads them — a
                 * post-compare xor would clobber ZF/CF and break the result. */
                emit_xor_rr(&out->text, REG_RDX);
                emit_sse_ucomi(&out->text, XMM_SCRATCH0, XMM_SCRATCH1,
                               is_float);
                emit_setcc_r(&out->text, cc, REG_RDX);
                if (dr >= 0 && dr != REG_RDX)
                    emit_mov_rr(&out->text, dr, REG_RDX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RDX, ra);
                break;
            }

            case IR_SITOFP: {
                if (value_is_ld(fn, inst->dst)) {
                    /* int→long double.  Load int into a GP reg, store into dst's
                     * slot, fild dword from the slot (pushes ld to st0), fstpt
                     * the result back into dst's slot.  Clobbers RAX/RCX. */
                    int pdr = (ra && inst->a >= 0 && inst->a < ra->num_values)
                              ? ra->reg[inst->a] : -1;
                    int src = (pdr >= 0) ? pdr : REG_RAX;
                    if (pdr < 0) emit_load_spill(&out->text, REG_RAX, spill_offset(ra->spill_slot[inst->a]));
                    emit_ld_from_gp_int(&out->text, src, inst->dst, ld_off);
                    break;
                }
                /* dst (float) = (float)a, a = signed int.  Load a into a GP
                 * scratch, cvtsi2sd, then optionally narrow to float. */
                ensure_reg(&out->text, inst->a, REG_RAX, ra);
                int is_64 = (inst->width == 8);
                emit_sse_cvtsi2sd(&out->text, XMM_SCRATCH0, REG_RAX, is_64);
                if (inst->width == 4)
                    emit_sse_cvtsd2ss(&out->text, XMM_SCRATCH0, XMM_SCRATCH0);
                if (dr >= 0 && dr != XMM_SCRATCH0)
                    emit_sse_mov_rr(&out->text, dr, XMM_SCRATCH0);
                spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                    ra_xmm, gp_spill_area);
                break;
            }

            case IR_FPTOSI: {
                if (value_is_ld(fn, inst->a)) {
                    /* long double→int.  Load ld a into st0, sub a small scratch
                     * on the stack, fisttp dword to it, read into a GP reg.  Uses
                     * 8 bytes below rsp (SysV red zone is safe here).  Clobbers
                     * RAX/RCX/RDX. */
                    emit_ld_load(&out->text, inst->a, ld_off); /* fldt [a] → st0 */
                    emit_sub_rsp_imm32(&out->text, 8);
                    /* lea rcx, [rsp] */
                    emit_byte(&out->text, 0x48); emit_byte(&out->text, 0x8D);
                    emit_modrm(&out->text, 0, REG_RCX & 7, 4);
                    emit_byte(&out->text, 0x24);
                    emit_byte(&out->text, 0xDB);
                    emit_modrm(&out->text, 0, 1, REG_RCX & 7); /* fisttp dword [rsp] */
                    int dr_gp = (ra && inst->dst >= 0 && inst->dst < ra->num_values)
                                ? ra->reg[inst->dst] : -1;
                    int dst_reg = (dr_gp >= 0) ? dr_gp : REG_RAX;
                    /* mov dst_reg, [rsp] */
                    emit_byte(&out->text, 0x48); emit_byte(&out->text, 0x8B);
                    emit_modrm(&out->text, 0, dst_reg & 7, 4);
                    emit_byte(&out->text, 0x24);
                    emit_add_rsp_imm32(&out->text, 8);
                    if (dr_gp >= 0 && dr_gp != dst_reg)
                        emit_mov_rr(&out->text, dr_gp, dst_reg);
                    spill_if_needed(&out->text, inst->dst,
                                    dr_gp >= 0 ? dr_gp : REG_RAX, ra);
                    break;
                }
                /* dst (int) = (int)a, a = float.  The source float width is
                 * inst->imm (inst->width is the target int width).  Load a
                 * into XMM scratch, cvtsd2si/cvtss2si into a GP scratch. */
                ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                               gp_spill_area);
                int src_float = (inst->imm == 4);
                int dst_64 = (inst->width == 8);
                if (src_float)
                    emit_sse_cvtss2si(&out->text, REG_RAX, XMM_SCRATCH0, dst_64);
                else
                    emit_sse_cvtsd2si(&out->text, REG_RAX, XMM_SCRATCH0, dst_64);
                if (dr >= 0 && dr != REG_RAX)
                    emit_mov_rr(&out->text, dr, REG_RAX);
                spill_if_needed(&out->text, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_FPEXT: {
                if (value_is_ld(fn, inst->dst)) {
                    /* double (or float)→long double.  Load the SSE value into
                     * XMM scratch, store to dst's slot, fld qword/dword from
                     * the slot (pushes ld to st0), fstpt back to dst's slot. */
                    ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                                   gp_spill_area);
                    emit_ld_addr(&out->text, REG_RCX, ld_off[inst->dst]);
                    emit_sse_store_via_ptr(&out->text, REG_RCX, XMM_SCRATCH0, 0);
                    emit_byte(&out->text, 0xDD); /* fld qword [rcx] */
                    emit_modrm(&out->text, 0, 0, REG_RCX & 7);
                    emit_x87_fstptRCX(&out->text); /* fstpt [rcx] */
                    break;
                }
                /* dst (double) = (double)(float)a.  Widen float→double. */
                ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                               gp_spill_area);
                emit_sse_cvtss2sd(&out->text, XMM_SCRATCH0, XMM_SCRATCH0);
                if (dr >= 0 && dr != XMM_SCRATCH0)
                    emit_sse_mov_rr(&out->text, dr, XMM_SCRATCH0);
                spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                    ra_xmm, gp_spill_area);
                break;
            }

            case IR_FPTRUNC: {
                if (value_is_ld(fn, inst->a)) {
                    /* long double→double (or float).  fldt ld source slot into
                     * st0, fstp qword into the source slot's first 8 bytes (the
                     * source is consumed, so reusing it as scratch is safe),
                     * load into XMM scratch → dst home. */
                    emit_ld_load(&out->text, inst->a, ld_off); /* fldt → st0 */
                    emit_ld_addr(&out->text, REG_RCX, ld_off[inst->a]);
                    emit_byte(&out->text, 0xDD); /* fstp qword [rcx] */
                    emit_modrm(&out->text, 0, 3, REG_RCX & 7);
                    emit_sse_load_via_ptr(&out->text, XMM_SCRATCH0, REG_RCX, 0);
                    if (dr >= 0 && dr != XMM_SCRATCH0)
                        emit_sse_mov_rr(&out->text, dr, XMM_SCRATCH0);
                    spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                        ra_xmm, gp_spill_area);
                    break;
                }
                /* dst (float) = (float)(double)a.  Narrow double→float. */
                ensure_reg_xmm(&out->text, inst->a, XMM_SCRATCH0, ra_xmm,
                               gp_spill_area);
                emit_sse_cvtsd2ss(&out->text, XMM_SCRATCH0, XMM_SCRATCH0);
                if (dr >= 0 && dr != XMM_SCRATCH0)
                    emit_sse_mov_rr(&out->text, dr, XMM_SCRATCH0);
                spill_if_needed_xmm(&out->text, inst->dst, XMM_SCRATCH0,
                                    ra_xmm, gp_spill_area);
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
            memcpy(out->text.data + p->patch_off, &rel32, 4);
        }
        free(patches);
        free(label_off);
        free(alloca_off);
        free(ld_off);

        size_t fn_size = out->text.len - start_offset;
        uint8_t binding = fn->is_static ? 0 /* STB_LOCAL */ : 1 /* STB_GLOBAL */;
        emit_module_add_symbol(out, fn->name, binding, 2 /* STT_FUNC */,
                               (uint16_t)SECT_TEXT, start_offset, fn_size);
    }

    /* ---- Resolve cross-function call patches ---- */
    for (size_t pi = 0; pi < num_call_patches; pi++) {
        CallPatch *cp = &call_patches[pi];
        /* Look up the callee among DEFINED symbols in this module.  A match
         * against an undefined symbol (forward declaration) must NOT resolve
         * here — it needs a relocation so the linker can bind it across TUs. */
        size_t target = (size_t)-1;
        for (size_t si = 0; si < out->num_syms; si++) {
            if (out->syms[si].name && strcmp(out->syms[si].name, cp->callee) == 0
                && out->syms[si].shndx != SECT_UNDEF) {
                target = out->syms[si].value;
                break;
            }
        }
        if (target == (size_t)-1) {
            /* Not a defined function — external (`extern`) call.  Record a
             * PLT32 relocation targeting the (undefined) symbol; the linker
             * resolves it to the symbol's address or a PLT entry. */
            int esym = emit_module_find_symbol(out, cp->callee);
            if (esym < 0)
                esym = emit_module_add_undefined(out, cp->callee);
            emit_module_add_reloc(out, cp->patch_off, R_X86_64_PLT32, esym, -4);
            free(cp->callee);
            continue;
        }
        int64_t rel = (int64_t)target - (int64_t)cp->after_off;
        if (rel < INT32_MIN || rel > INT32_MAX) {
            fprintf(stderr, "fakecc: call displacement out of range\n");
            exit(1);
        }
        int32_t rel32 = (int32_t)rel;
        memcpy(out->text.data + cp->patch_off, &rel32, 4);
        free(cp->callee);
    }
    free(call_patches);

    /* ---- Resolve function-address load patches ---- */
    for (size_t pi = 0; pi < num_fnaddr_patches; pi++) {
        FnAddrPatch *fp = &fnaddr_patches[pi];
        /* Look up the function in the symbol table. */
        size_t target = (size_t)-1;
        for (size_t si = 0; si < out->num_syms; si++) {
            if (out->syms[si].name && strcmp(out->syms[si].name, fp->fn_name) == 0
                && out->syms[si].shndx != SECT_UNDEF) {
                target = out->syms[si].value;
                break;
            }
        }
        if (target == (size_t)-1) {
            /* External function address — record a PC32 relocation targeting
             * the undefined symbol; the linker resolves it. */
            int esym = emit_module_find_symbol(out, fp->fn_name);
            if (esym < 0)
                esym = emit_module_add_undefined(out, fp->fn_name);
            emit_module_add_reloc(out, fp->patch_off, R_X86_64_PC32, esym, -4);
            free(fp->fn_name);
            continue;
        }
        /* lea r, [rip+disp32]: disp32 = target - (rip_next) where both are
         * absolute addresses.  target = ELF_BASE + symbol value;
         * rip_next = ELF_BASE + patch_off + 4.  The ELF_BASE cancels: */
        int64_t rel = (int64_t)target - (int64_t)(fp->patch_off + 4);
        if (rel < INT32_MIN || rel > INT32_MAX) {
            fprintf(stderr, "fakecc: function address displacement out of range\n");
            exit(1);
        }
        int32_t rel32 = (int32_t)rel;
        memcpy(out->text.data + fp->patch_off, &rel32, 4);
        free(fp->fn_name);
    }
    free(fnaddr_patches);
}
