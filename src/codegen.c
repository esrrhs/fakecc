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
    size_t patch_off;     /* absolute offset of the rel32 field in out->code */
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
    /* Emit globals into the data buffer.  Each global is written as its
     * initializer bytes (or zeros if no init) and registered by name. */
    for (size_t gi = 0; gi < ir->globals.len; gi++) {
        const IRGlobal *g = &ir->globals.data[gi];
        size_t off = out->data.len;
        if (g->init_bytes) buffer_append(&out->data, g->init_bytes, g->size);
        else {
            /* Zero-fill. */
            for (int b = 0; b < g->size; b++) {
                char z = 0;
                buffer_append(&out->data, &z, 1);
            }
        }
        emit_module_add_global(out, g->name, off, g->size, g->is_readonly);
        /* Pad to 8-byte boundary so the next global is aligned. */
        while (out->data.len & 7) {
            char z = 0;
            buffer_append(&out->data, &z, 1);
        }
    }

    /* Cross-function call patches (accumulated across every function). */
    CallPatch *call_patches = NULL;
    size_t num_call_patches = 0, cap_call_patches = 0;

    for (size_t i = 0; i < ir->functions.len; i++) {
        const IRFunction *fn = &ir->functions.data[i];
        const RAResult *ra = (const RAResult *)fn->ra;
        size_t start_offset = out->code.len;

        /* ---- Prologue ---- */
        int cs_used[3];
        collect_callee_saved(ra, cs_used);

        /* Compute pinned-alloca frame area: byte-offset from rbp for each
         * IR_ALLOCA with alloca_bytes > 0.  Offsets are placed above spill
         * area (further from rbp, i.e. more negative). */
        int *alloca_off = xmalloc(fn->next_value_id * sizeof(int));
        for (int i = 0; i < fn->next_value_id; i++) alloca_off[i] = 0;
        int spill_area = ra ? ra->stack_size : 0;
        int pinned_area = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            const IRInst *inst = &fn->insts.data[j];
            if (inst->op == IR_ALLOCA && inst->alloca_bytes > 0 && inst->dst >= 0) {
                int bytes = inst->alloca_bytes;
                if (bytes % 8 != 0) bytes += 8 - (bytes % 8);
                pinned_area += bytes;
                alloca_off[inst->dst] = -(spill_area + pinned_area);
            }
        }

        int stack_size = spill_area + pinned_area;
        if (!ra && stack_size == 0) stack_size = 8 * fn->next_value_id;
        if (stack_size % 16 != 0) stack_size += 16 - (stack_size % 16);

        /* Alignment invariant: on entry rsp % 16 == 8 (call pushed ret).
         * After "push rbp" rsp % 16 == 0.  Each callee-saved push adds 8;
         * if odd count, we need one extra bump to keep rsp aligned. */
        int cs_count = cs_used[0] + cs_used[1] + cs_used[2];
        if (((cs_count) & 1) != 0) stack_size += 8;

        emit_byte(&out->code, 0x55);              /* pushq %rbp */
        emit_rex_w(&out->code);
        emit_byte(&out->code, 0x89);
        emit_byte(&out->code, 0xE5);              /* movq %rsp, %rbp */

        if (cs_used[0]) emit_push_r(&out->code, REG_RBX);
        if (cs_used[1]) emit_push_r(&out->code, REG_R12);
        if (cs_used[2]) emit_push_r(&out->code, REG_R13);

        /* Materialize incoming SysV arg registers into their allocated
         * homes.  Use a push-then-pop dance so no arg-reg clobber can
         * lose data even when a param's home is another param's src reg. */
        int nparams = 0;
        for (size_t j = 0; j < fn->insts.len; j++) {
            if (fn->insts.data[j].op == IR_PARAM) nparams++;
            else break;  /* IR_PARAMs are contiguous at function start */
        }
        int nreg_params = nparams > 6 ? 6 : nparams;
        /* Push register args in REVERSE order (so arg 0 ends up on top). */
        for (int p = nreg_params - 1; p >= 0; p--) {
            emit_push_r(&out->code, SYSV_ARG_REGS[p]);
        }
        /* Pop into each param's allocated home (or spill slot) in order. */
        for (int p = 0; p < nreg_params; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                      ? ra->reg[pi->dst] : -1;
            if (pdr >= 0) {
                emit_pop_r(&out->code, pdr);
            } else {
                emit_pop_r(&out->code, REG_RAX);
                spill_if_needed(&out->code, pi->dst, REG_RAX, ra);
            }
        }
        /* Load stack-passed args from [rbp + 16 + 8*(k-6)] into their homes. */
        for (int p = 6; p < nparams; p++) {
            const IRInst *pi = &fn->insts.data[p];
            int pdr = (ra && pi->dst >= 0 && pi->dst < ra->num_values)
                      ? ra->reg[pi->dst] : -1;
            int off = 16 + 8 * (p - 6);
            if (pdr >= 0) {
                emit_load_spill(&out->code, pdr, off);
            } else {
                emit_load_spill(&out->code, REG_RAX, off);
                spill_if_needed(&out->code, pi->dst, REG_RAX, ra);
            }
        }

        emit_sub_rsp_imm32(&out->code, stack_size); /* sub $N, %rsp */

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
                /* Stage both operands into scratch (rax = a, rcx = b) so that
                 * loading `a` into `dr` can't clobber `b` if reg[b] == dr. */
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                emit_add_rr(&out->code, REG_RAX, REG_RCX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_SUB: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                emit_sub_rr(&out->code, REG_RAX, REG_RCX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_MUL: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                emit_imul_rr(&out->code, REG_RAX, REG_RCX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_DIV:
            case IR_MOD: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                if (inst->is_unsigned) {
                    /* Unsigned: zero-extend rax into rdx then div. */
                    emit_xor_rr(&out->code, REG_RDX);
                    /* div %rcx: F7 /6 */
                    emit_rex_w(&out->code);
                    emit_byte(&out->code, 0xF7);
                    emit_byte(&out->code, 0xF1);
                } else {
                    emit_cqto(&out->code);
                    emit_idiv_rcx(&out->code);
                }
                if (inst->op == IR_DIV) {
                    mask_to_width(&out->code, REG_RAX, inst->width);
                    if (dr >= 0 && dr != REG_RAX)
                        emit_mov_rr(&out->code, dr, REG_RAX);
                    spill_if_needed(&out->code, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                } else {
                    mask_to_width(&out->code, REG_RDX, inst->width);
                    if (dr >= 0)
                        emit_mov_rr(&out->code, dr, REG_RDX);
                    spill_if_needed(&out->code, inst->dst,
                                    dr >= 0 ? dr : REG_RAX, ra);
                }
                break;
            }

            case IR_NEG: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                emit_neg_r(&out->code, REG_RAX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_BNOT: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                emit_not_r(&out->code, REG_RAX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_BAND:
            case IR_BOR:
            case IR_BXOR: {
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                if (inst->op == IR_BAND)
                    emit_and_rr(&out->code, REG_RAX, REG_RCX);
                else if (inst->op == IR_BOR)
                    emit_or_rr(&out->code, REG_RAX, REG_RCX);
                else
                    emit_bitxor_rr(&out->code, REG_RAX, REG_RCX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
                                dr >= 0 ? dr : REG_RAX, ra);
                break;
            }

            case IR_SHL:
            case IR_SHR: {
                /* Shift count must be in cl. Ensure b into rcx FIRST (before
                 * a into rax) only if a isn't bound to rcx — but ensure_reg
                 * for a uses rax, so order is safe: a→rax, b→rcx. */
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                if (inst->op == IR_SHL)
                    emit_shl_rcx(&out->code, REG_RAX);
                else if (inst->is_unsigned)
                    emit_shr_rcx(&out->code, REG_RAX);
                else
                    emit_sar_rcx(&out->code, REG_RAX);
                mask_to_width(&out->code, REG_RAX, inst->width);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
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
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                ensure_reg(&out->code, inst->b, REG_RCX, ra);
                uint8_t cc = ir_cmp_to_setcc(inst->op, inst->is_unsigned);
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

            case IR_COPY: {
                if (ra && inst->a >= 0 && inst->a < ra->num_values &&
                    inst->dst >= 0 && inst->dst < ra->num_values &&
                    ra->reg[inst->dst] == ra->reg[inst->a] &&
                    ra->reg[inst->dst] >= 0) {
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

            case IR_SEXT:
            case IR_ZEXT: {
                /* imm = source width */
                int src_w = inst->imm;
                ensure_reg(&out->code, inst->a, REG_RAX, ra);
                if (inst->op == IR_SEXT)
                    emit_movsx_rr(&out->code, REG_RAX, REG_RAX, src_w);
                else
                    emit_movzx_rr(&out->code, REG_RAX, REG_RAX, src_w);
                if (dr >= 0 && dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                spill_if_needed(&out->code, inst->dst,
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
                    ensure_reg(&out->code, inst->a, dr, ra);
                } else {
                    old_load(&out->code, inst->a, REG_RAX);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_LOAD: {
                /* Rarely reached: mem2reg usually eliminates LOAD.  If a LOAD
                 * survives, treat it as a copy of the alloca's spill slot
                 * with narrow-width sign/zero-extend semantics.  We reuse
                 * the same copy fast-path since spill layout is 8B/slot. */
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
                    emit_lea_rbp(&out->code, dr, off);
                } else {
                    emit_lea_rbp(&out->code, REG_RAX, off);
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                }
                break;
            }

            case IR_GADDR: {
                /* dst = &global; target name in inst->call_name.  Emit
                 * `lea r, [rip+0]` and register a fixup for post-layout patching. */
                int target = dr >= 0 ? dr : REG_RAX;
                size_t patch = emit_lea_rip(&out->code, target);
                emit_module_add_reloc(out, patch, inst->call_name);
                if (dr < 0)
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                break;
            }

            case IR_LOAD_PTR: {
                /* dst = *ptr.  ptr = inst->a. */
                ensure_reg(&out->code, inst->a, REG_RCX, ra);
                emit_load_via_ptr(&out->code,
                                  dr >= 0 ? dr : REG_RAX,
                                  REG_RCX, inst->width, inst->is_unsigned);
                if (dr < 0)
                    spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                break;
            }

            case IR_STORE_PTR: {
                /* *ptr = val.  ptr = inst->a, val = inst->b. */
                ensure_reg(&out->code, inst->a, REG_RCX, ra);
                ensure_reg(&out->code, inst->b, REG_RAX, ra);
                emit_store_via_ptr(&out->code, REG_RCX, REG_RAX, inst->width);
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
                        ensure_reg(&out->code, inst->call_args[k], REG_RCX, ra);
                        emit_push_r(&out->code, REG_RCX);
                    }
                    /* Pop into syscall arg regs in reverse (arg 0 = num → RAX) */
                    for (int k = nargs - 1; k >= 0; k--) {
                        emit_pop_r(&out->code, SYS_ARG_REGS[k]);
                    }
                    /* Emit `syscall` — 0F 05 */
                    emit_byte(&out->code, 0x0F);
                    emit_byte(&out->code, 0x05);
                    /* Result in RAX; move to dst or spill. */
                    if (dr >= 0) {
                        if (dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                    } else {
                        spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                    }
                    break;
                }
                int nargs = inst->call_nargs;
                int nreg  = nargs > 6 ? 6 : nargs;
                int nstack = nargs > 6 ? nargs - 6 : 0;
                /* Alignment: at call time rsp must be 16-aligned.  Function
                 * prologue keeps rsp 16-aligned in the body; each 8-byte
                 * push we do here shifts it by 8.  Total pushes we do
                 * before the call: nstack (for stack args). The 6 reg
                 * arg pushes are balanced by 6 pops.  So we need to
                 * pad iff (nstack % 2) != 0. */
                int need_pad = (nstack & 1);
                if (need_pad) emit_sub_rsp_imm32(&out->code, 8);

                /* Push stack args right-to-left (arg N-1 first → arg 6 on top
                 * closest to callee's rbp+16). */
                for (int k = nargs - 1; k >= 6; k--) {
                    ensure_reg(&out->code, inst->call_args[k], REG_RAX, ra);
                    emit_push_r(&out->code, REG_RAX);
                }

                /* Register args: push all in order (arg 0 first), then pop
                 * into arg regs in reverse. This gives the push-then-pop
                 * dance behaviour that dodges clobbers. */
                for (int k = 0; k < nreg; k++) {
                    ensure_reg(&out->code, inst->call_args[k], REG_RAX, ra);
                    emit_push_r(&out->code, REG_RAX);
                }
                for (int k = nreg - 1; k >= 0; k--) {
                    emit_pop_r(&out->code, SYSV_ARG_REGS[k]);
                }

                /* Emit call rel32 with a cross-function patch. */
                size_t poff = emit_call_rel32(&out->code);
                size_t aft = out->code.len;
                if (num_call_patches >= cap_call_patches) {
                    cap_call_patches = cap_call_patches ? cap_call_patches * 2 : 8;
                    call_patches = xrealloc(call_patches,
                                             cap_call_patches * sizeof(CallPatch));
                }
                call_patches[num_call_patches].patch_off = poff;
                call_patches[num_call_patches].callee = xstrdup(inst->call_name);
                call_patches[num_call_patches].after_off = aft;
                num_call_patches++;

                /* Tear down stack args + padding. */
                int cleanup = nstack * 8 + (need_pad ? 8 : 0);
                if (cleanup > 0) emit_add_rsp_imm32(&out->code, cleanup);

                /* Result is in RAX; move to dst home (or spill).  A void call
                 * has dst == -1 (no result) — leave RAX alone. */
                if (inst->dst >= 0) {
                    if (dr >= 0) {
                        if (dr != REG_RAX) emit_mov_rr(&out->code, dr, REG_RAX);
                    } else {
                        spill_if_needed(&out->code, inst->dst, REG_RAX, ra);
                    }
                }
                break;
            }

            case IR_RETURN: {
                /* Void function: bare `return;` — no value in a. */
                if (inst->a != -1)
                    ensure_reg(&out->code, inst->a, REG_RAX, ra);
                emit_epilogue(&out->code, stack_size, cs_used);
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
        free(alloca_off);

        size_t fn_size = out->code.len - start_offset;
        emit_module_add_symbol(out, fn->name, start_offset, fn_size);
    }

    /* ---- Resolve cross-function call patches ---- */
    for (size_t pi = 0; pi < num_call_patches; pi++) {
        CallPatch *cp = &call_patches[pi];
        /* Look up the callee's start offset in the module's symbol table. */
        size_t target = (size_t)-1;
        for (size_t si = 0; si < out->num_symbols; si++) {
            if (strcmp(out->symbols[si].name, cp->callee) == 0) {
                target = out->symbols[si].offset;
                break;
            }
        }
        if (target == (size_t)-1) {
            fprintf(stderr, "fakecc: unresolved call to '%s'\n", cp->callee);
            exit(1);
        }
        int64_t rel = (int64_t)target - (int64_t)cp->after_off;
        if (rel < INT32_MIN || rel > INT32_MAX) {
            fprintf(stderr, "fakecc: call displacement out of range\n");
            exit(1);
        }
        int32_t rel32 = (int32_t)rel;
        memcpy(out->code.data + cp->patch_off, &rel32, 4);
        free(cp->callee);
    }
    free(call_patches);
}
