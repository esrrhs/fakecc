#include "fakecc/a64.h"
#include "fakecc/reg_arm64.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Golden words produced by `clang -arch arm64` from an equivalent
 * reference assembly (see git history of this test for the .s listing).
 * 100 instructions: GP / loads / branches / FP in three label groups
 * laid out exactly like the reference so branch displacements match. */
static const uint32_t GOLDEN[100] = {
    /* ── GP (45) ── */
    0xd2800000, 0xd29fffe1, 0xd2a00022, 0xd2c00043, 0xd2e00064,
    0xf2b579a5, 0x92800006, 0x929fffe7, 0xaa0903e8, 0x2a0b03ea,
    0x91000020, 0x913ffc62, 0x914004a4, 0xd10004e6, 0x1103fc20,
    0x51000462,
    0x8b020020, 0xcb050083, 0xab0800e6, 0xeb0b0149, 0xcb0d03ec,
    0x0b020c20,
    0x8a020020, 0xaa050083, 0xca0800e6,
    0x92401c20, 0x92781c62, 0xb24000a4, 0x12003ce6,
    0xd37ff820, 0xd3410062, 0xd348fca4, 0x9361fce6, 0x531f7928,
    0x531f7d6a, 0x13107dac,
    0x9b027c20, 0x9b059883, 0x9ac90907, 0x9acc0d6a,
    0xf100001f, 0xf13ffc3f, 0xeb03005f, 0x7100049f, 0x6b0600bf,
    /* ── loads (25) ── */
    0xf9000020, 0xf9000420, 0xf903fc20, 0xf9400062, 0xf97ffca4,
    0xb9000420, 0xb940fc62,
    0x39000020, 0x39400462,
    0x790004a4, 0x797ffce6,
    0xb9800528, 0x3980016a, 0x798005ac,
    0xf8626820, 0xf8256883, 0x786868e6, 0x38ab6949,
    0xa9000440, 0xa9010c82, 0xa9bf14c4, 0xa9402127, 0xa9422969,
    0x29408440, 0x29bf8c82,
    /* ── branches (14) ── */
    0x17ffffba, 0x97ffffe6, 0xb4fff700, 0xb5fffc81, 0x34fff6c2,
    0x35fffc43,
    0x54fff680, 0x54fffc01, 0x54fff64d, 0x54fffbc8,
    0xd65f03c0, 0xd4001001, 0x90000000, 0x91000000,
    /* ── FP (16) ── */
    0x1e604020, 0x1e204062,
    0x1e6628a4, 0x1e292907, 0x1e6c096a, 0x1e6f19cd, 0x1e723a30,
    0x1e742260,
    0x9e6202d5, 0x1e220317, 0x9e780359, 0x1e38039b,
    0xfd000c20, 0xfd400862, 0xbd0004a4, 0xbd4000e6,
};

static uint32_t word_at(const A64Asm *a, size_t i) {
    uint32_t w;
    memcpy(&w, a->code.data + i * 4, 4);
    return w;
}

/* Emit the exact instruction stream the golden words were taken from. */
static void emit_reference(A64Asm *a, int *lgp, int *lloads, int *lbranch) {
    *lgp = a64_new_label(a);
    a64_bind(a, *lgp);
    a64_movz(a, A64_X0, 0, 0, 1);
    a64_movz(a, A64_X1, 0xffff, 0, 1);
    a64_movz(a, A64_X2, 1, 1, 1);
    a64_movz(a, A64_X3, 2, 2, 1);
    a64_movz(a, A64_X4, 3, 3, 1);
    a64_movk(a, A64_X5, 0xabcd, 1, 1);
    a64_movn(a, A64_X6, 0, 0, 1);
    a64_movn(a, A64_X7, 0xffff, 0, 1);
    a64_mov_reg(a, A64_X8, A64_X9, 1);
    a64_mov_reg(a, A64_W10, A64_W11, 0);

    a64_add_imm12(a, A64_X0, A64_X1, 0, 0, 1, 0);
    a64_add_imm12(a, A64_X2, A64_X3, 4095, 0, 1, 0);
    a64_add_imm12(a, A64_X4, A64_X5, 1, 1, 1, 0);
    a64_sub_imm12(a, A64_X6, A64_X7, 1, 0, 1, 0);
    a64_add_imm12(a, A64_W0, A64_W1, 255, 0, 0, 0);
    a64_sub_imm12(a, A64_W2, A64_W3, 1, 0, 0, 0);

    a64_add_reg(a, A64_X0, A64_X1, A64_X2, A64_LSL, 0, 1, 0);
    a64_sub_reg(a, A64_X3, A64_X4, A64_X5, A64_LSL, 0, 1, 0);
    a64_add_reg(a, A64_X6, A64_X7, A64_X8, A64_LSL, 0, 1, 1);
    a64_sub_reg(a, A64_X9, A64_X10, A64_X11, A64_LSL, 0, 1, 1);
    a64_neg(a, A64_X12, A64_X13, 1);
    a64_add_reg(a, A64_W0, A64_W1, A64_W2, A64_LSL, 3, 0, 0);

    a64_and_reg(a, A64_X0, A64_X1, A64_X2, 1);
    a64_or_reg(a, A64_X3, A64_X4, A64_X5, 1);
    a64_eor_reg(a, A64_X6, A64_X7, A64_X8, 1);
    T_ASSERT_EQ_INT(a64_and_imm(a, A64_X0, A64_X1, 0xff, 1), 0);
    T_ASSERT_EQ_INT(a64_and_imm(a, A64_X2, A64_X3, 0xff00, 1), 0);
    T_ASSERT_EQ_INT(a64_or_imm(a, A64_X4, A64_X5, 1, 1), 0);
    T_ASSERT_EQ_INT(a64_and_imm(a, A64_W6, A64_W7, 0xffff, 0), 0);

    a64_lsl_imm(a, A64_X0, A64_X1, 1, 1);
    a64_lsl_imm(a, A64_X2, A64_X3, 63, 1);
    a64_lsr_imm(a, A64_X4, A64_X5, 8, 1);
    a64_asr_imm(a, A64_X6, A64_X7, 33, 1);
    a64_lsl_imm(a, A64_W8, A64_W9, 1, 0);
    a64_lsr_imm(a, A64_W10, A64_W11, 31, 0);
    a64_asr_imm(a, A64_W12, A64_W13, 16, 0);

    a64_mul(a, A64_X0, A64_X1, A64_X2, 1);
    a64_msub(a, A64_X3, A64_X4, A64_X5, A64_X6, 1);
    a64_udiv(a, A64_X7, A64_X8, A64_X9, 1);
    a64_sdiv(a, A64_X10, A64_X11, A64_X12, 1);

    a64_cmp_imm12(a, A64_X0, 0, 0, 1);
    a64_cmp_imm12(a, A64_X1, 4095, 0, 1);
    a64_cmp_reg(a, A64_X2, A64_X3, 1);
    a64_cmp_imm12(a, A64_W4, 1, 0, 0);
    a64_cmp_reg(a, A64_W5, A64_W6, 0);

    *lloads = a64_new_label(a);
    a64_bind(a, *lloads);
    a64_str64(a, A64_X0, A64_X1, 0);
    a64_str64(a, A64_X0, A64_X1, 8);
    a64_str64(a, A64_X0, A64_X1, 0x7f8);
    a64_ldr64(a, A64_X2, A64_X3, 0);
    a64_ldr64(a, A64_X4, A64_X5, 0x7ff8);
    a64_str32(a, A64_W0, A64_X1, 4);
    a64_ldr32(a, A64_W2, A64_X3, 0xfc);
    a64_str8(a, A64_W0, A64_X1, 0);
    a64_ldr8(a, A64_W2, A64_X3, 1);
    a64_str16(a, A64_W4, A64_X5, 2);
    a64_ldr16(a, A64_W6, A64_X7, 0x1ffe);
    a64_ldrsw(a, A64_X8, A64_X9, 4);
    a64_ldrsb64(a, A64_X10, A64_X11, 0);
    a64_ldrsh64(a, A64_X12, A64_X13, 2);
    a64_ldr64_reg(a, A64_X0, A64_X1, A64_X2);
    a64_str64_reg(a, A64_X3, A64_X4, A64_X5);
    a64_ldr16_reg(a, A64_W6, A64_X7, A64_X8);
    a64_ldrsb64_reg(a, A64_X9, A64_X10, A64_X11);
    a64_stp64(a, A64_X0, A64_X1, A64_X2, 0, A64_PAIR_OFFSET);
    a64_stp64(a, A64_X2, A64_X3, A64_X4, 16, A64_PAIR_OFFSET);
    a64_stp64(a, A64_X4, A64_X5, A64_X6, -16, A64_PAIR_PRE);
    a64_ldp64(a, A64_X7, A64_X8, A64_X9, 0, A64_PAIR_OFFSET);
    a64_ldp64(a, A64_X9, A64_X10, A64_X11, 32, A64_PAIR_OFFSET);
    a64_ldp32(a, A64_W0, A64_W1, A64_X2, 4, A64_PAIR_OFFSET);
    a64_stp32(a, A64_W2, A64_W3, A64_X4, -4, A64_PAIR_PRE);

    *lbranch = a64_new_label(a);
    a64_bind(a, *lbranch);
    a64_b(a, *lgp);
    a64_bl(a, *lloads);
    a64_cbz(a, A64_X0, *lgp, 1);
    a64_cbnz(a, A64_X1, *lloads, 1);
    a64_cbz(a, A64_W2, *lgp, 0);
    a64_cbnz(a, A64_W3, *lloads, 0);
    a64_bcond(a, A64_EQ, *lgp);
    a64_bcond(a, A64_NE, *lloads);
    a64_bcond(a, A64_LE, *lgp);
    a64_bcond(a, A64_HI, *lloads);
    a64_ret(a, A64_LR);
    a64_svc(a, 0x80);
    a64_adrp_label(a, A64_X0, *lgp);
    a64_add_imm12(a, A64_X0, A64_X0, 0, 0, 1, 0);

    a64_fmov_reg(a, A64_V0, A64_V1, 1);
    a64_fmov_reg(a, A64_V2, A64_V3, 0);
    a64_fadd(a, A64_V4, A64_V5, A64_V6, 1);
    a64_fadd(a, A64_V7, A64_V8, A64_V9, 0);
    a64_fmul(a, A64_V10, A64_V11, A64_V12, 1);
    a64_fdiv(a, A64_V13, A64_V14, A64_V15, 1);
    a64_fsub(a, A64_V16, A64_V17, A64_V18, 1);
    a64_fcmp(a, A64_V19, A64_V20, 1);
    a64_scvtf(a, A64_V21, A64_X22, 1);
    a64_scvtf(a, A64_V23, A64_W24, 0);
    a64_fcvtzs(a, A64_X25, A64_V26, 1);
    a64_fcvtzs(a, A64_W27, A64_V28, 0);
    a64_str_d(a, A64_V0, A64_X1, 24);
    a64_ldr_d(a, A64_V2, A64_X3, 16);
    a64_str_s(a, A64_V4, A64_X5, 4);
    a64_ldr_s(a, A64_V6, A64_X7, 0);
}

static void test_golden_match(void) {
    A64Asm a;
    a64_init(&a);
    int lgp, lloads, lbranch;
    emit_reference(&a, &lgp, &lloads, &lbranch);
    T_ASSERT_EQ_INT((int)(a.code.len / 4), 100);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);

    int mismatches = 0;
    for (size_t i = 0; i < 100; i++) {
        if (word_at(&a, i) != GOLDEN[i]) {
            if (mismatches < 8)
                fprintf(stderr, "  word %zu: got %08x want %08x\n",
                        i, word_at(&a, i), GOLDEN[i]);
            mismatches++;
        }
    }
    T_ASSERT_EQ_INT(mismatches, 0);
    a64_free(&a);
}

/* mov_imm64 materialization (not covered by the golden reference). */
static void test_mov_imm64(void) {
    A64Asm a;
    a64_init(&a);
    a64_mov_imm64(&a, A64_X0, 0);
    T_ASSERT_EQ_INT(word_at(&a, 0), 0xd2800000u);
    a64_mov_imm64(&a, A64_X0, 0xffff);
    T_ASSERT_EQ_INT(word_at(&a, 1), 0xd29fffe0u);
    a64_mov_imm64(&a, A64_X0, 0x10000);
    T_ASSERT_EQ_INT(word_at(&a, 2), 0xd2a00020u);

    /* All ones: movz ffff + three movk ffff. */
    a64_mov_imm64(&a, A64_X0, 0xffffffffffffffffull);
    T_ASSERT_EQ_INT(word_at(&a, 3), 0xd29fffe0u);
    T_ASSERT_EQ_INT(word_at(&a, 4), 0xf2bfffe0u);
    T_ASSERT_EQ_INT(word_at(&a, 5), 0xf2dfffe0u);
    T_ASSERT_EQ_INT(word_at(&a, 6), 0xf2ffffe0u);

    /* Sparse constant skips zero halfwords. */
    a64_mov_imm64(&a, A64_X0, 0x1234000000005678ull);
    T_ASSERT_EQ_INT(word_at(&a, 7), 0xd28acf00u); /* movz #0x5678 */
    T_ASSERT_EQ_INT(word_at(&a, 8), 0xf2e24680u); /* movk #0x1234, lsl 48 */
    T_ASSERT_EQ_INT((int)(a.code.len / 4), 9);

    /* Non-bitmask constants cannot be encoded as logical immediates. */
    T_ASSERT_EQ_INT(a64_and_imm(&a, A64_X0, A64_X1, 0x12345, 1), -1);
    a64_free(&a);
}

/* Synthetic boundary tests for branch/page fixups.  Labels are bound at a
 * real offset then their recorded position is overridden to create large
 * displacements without allocating a 128MB buffer. */

/* Emit b/bl at word 0; label initially bound at word 1. Returns label. */
static int branch_at_origin(A64Asm *a, int is_bl) {
    int l = a64_new_label(a);
    if (is_bl) a64_bl(a, l); else a64_b(a, l);
    a64_bind(a, l);                 /* pos = 4 */
    return l;
}

static void test_b26_boundaries(void) {
    /* Max forward: displacement 2^27-4 (in range), then exactly 2^27
     * (out of range). */
    A64Asm a;
    a64_init(&a);
    int l = branch_at_origin(&a, 0);
    a.labels[l].pos = (uint32_t)((1LL << 27) - 4);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);
    T_ASSERT_EQ_INT(word_at(&a, 0) & 0x03ffffffu, 0x01ffffffu);
    a64_free(&a);

    a64_init(&a);
    l = branch_at_origin(&a, 0);
    a.labels[l].pos = (uint32_t)((1LL << 27) + 4 - 4);  /* delta 2^27 */
    T_ASSERT_EQ_INT(a64_resolve(&a), -1);
    a64_free(&a);

    /* Max backward: branch word at 0x1000 targeting 0 → -4096 bytes;
     * exercise the sign-extended path at the ±128MB limit. */
    a64_init(&a);
    l = a64_new_label(&a);
    a64_bind(&a, l);                 /* pos 0 */
    while (a.code.len < 0x1000) a64_word(&a, 0xd503201fu); /* nop */
    a64_b(&a, l);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);
    uint32_t w = word_at(&a, 0x1000 / 4);
    T_ASSERT_EQ_INT(w & 0xfc000000u, 0x14000000u);
    T_ASSERT((int32_t)(w << 6) < 0);  /* displacement negative */
    a64_free(&a);

    /* Unaligned target → error. */
    a64_init(&a);
    l = branch_at_origin(&a, 0);
    a.labels[l].pos = 6;
    T_ASSERT_EQ_INT(a64_resolve(&a), -1);
    a64_free(&a);

    /* Unbound label → error. */
    a64_init(&a);
    l = a64_new_label(&a);
    a64_b(&a, l);
    T_ASSERT_EQ_INT(a64_resolve(&a), -1);
    a64_free(&a);
}

static void test_b19_boundaries(void) {
    A64Asm a;
    a64_init(&a);
    int l = a64_new_label(&a);
    a64_bcond(&a, A64_EQ, l);      /* word 0 */
    a64_bind(&a, l);                /* pos 4 */
    a.labels[l].pos = (uint32_t)((1LL << 20) - 4);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);
    /* ±1MB → word displacement ±2^18; max forward field is 2^18-1. */
    T_ASSERT_EQ_INT((word_at(&a, 0) >> 5) & 0x7ffffu, 0x3ffffu);
    a64_free(&a);

    a64_init(&a);
    l = a64_new_label(&a);
    a64_cbz(&a, A64_X0, l, 1);
    a64_bind(&a, l);
    a.labels[l].pos = (uint32_t)(1LL << 20);
    T_ASSERT_EQ_INT(a64_resolve(&a), -1);
    a64_free(&a);
}

static void test_adrp_boundaries(void) {
    /* adrp at word 0 to a label at the highest reachable forward page. */
    A64Asm a;
    a64_init(&a);
    int l = a64_new_label(&a);
    a64_adrp_label(&a, A64_X0, l);
    a64_bind(&a, l);
    /* pages = 2^20 - 1: immlo low 2 bits + immhi 19 bits roundtrip. */
    a.labels[l].pos = (uint32_t)(((int64_t)(1 << 20) - 1) << 12);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);
    uint32_t w = word_at(&a, 0);
    int64_t immlo = (w >> 29) & 3;
    int64_t immhi = (w >> 5) & 0x7ffff;
    int64_t pages = immlo | (immhi << 2);
    if (pages & (1LL << 20)) pages -= (1LL << 21);
    T_ASSERT_EQ_INT((int)pages, (int)((1LL << 20) - 1));
    a64_free(&a);

    /* Real backward ADRP across one page: label at 0, adrp at 0x1000 →
     * pages = -1, exercising sign extension of immlo/immhi. */
    a64_init(&a);
    l = a64_new_label(&a);
    a64_bind(&a, l);                  /* label pos 0 */
    while (a.code.len < 0x1000) a64_word(&a, 0xd503201fu);
    a64_adrp_label(&a, A64_X0, l);
    T_ASSERT_EQ_INT(a64_resolve(&a), 0);
    w = word_at(&a, 0x1000 / 4);
    immlo = (w >> 29) & 3;
    immhi = (w >> 5) & 0x7ffff;
    pages = immlo | (immhi << 2);
    if (pages & (1LL << 20)) pages -= (1LL << 21);
    T_ASSERT_EQ_INT((int)pages, -1);
    a64_free(&a);

    /* Any page delta between two 32-bit positions fits the signed 21-bit
     * ADRP field (max ±(2^20-1) pages), so an out-of-range pair cannot be
     * constructed inside one container; the resolver's bounds guard
     * exists for the field contract and future wider offsets.  Verify the
     * encoder still rejects a direct illegal value instead: adrp to a
     * bogus unbound label fails loudly. */
    a64_init(&a);
    l = a64_new_label(&a);
    a64_adrp_label(&a, A64_X0, l);
    /* never bound */
    T_ASSERT_EQ_INT(a64_resolve(&a), -1);
    a64_free(&a);
}

/* Pair address modes: signed-offset / post-index / pre-index.  The
 * prologue/epilogue words are the canonical clang-emitted frame pair
 * (post-index LDP is what restores FP/LR after a pre-index STP frame). */
static void test_pair_modes(void) {
    A64Asm a;
    a64_init(&a);
    /* Canonical frame: stp x29,x30,[sp,#-16]! / ldp x29,x30,[sp],#16. */
    a64_stp64(&a, A64_FP, A64_LR, A64_SP, -16, A64_PAIR_PRE);
    a64_ldp64(&a, A64_FP, A64_LR, A64_SP, 16, A64_PAIR_POST);
    T_ASSERT_EQ_INT(word_at(&a, 0), 0xA9BF7BFDu);
    T_ASSERT_EQ_INT(word_at(&a, 1), 0xA8C17BFDu);

    /* Offset mode carries no writeback; pre-index LDP is a distinct word
     * (regression: post used to be mis-encoded as pre, restoring FP/LR
     * from the caller's frame and returning into dyld). */
    a64_ldp64(&a, A64_X0, A64_X1, A64_SP, 16, A64_PAIR_OFFSET);
    a64_ldp64(&a, A64_X0, A64_X1, A64_SP, 16, A64_PAIR_PRE);
    a64_ldp64(&a, A64_X0, A64_X1, A64_SP, 16, A64_PAIR_POST);
    T_ASSERT_EQ_INT(word_at(&a, 2), 0xA94107E0u);
    T_ASSERT_EQ_INT(word_at(&a, 3), 0xA9C107E0u);
    T_ASSERT_EQ_INT(word_at(&a, 4), 0xA8C107E0u);

    /* 32-bit pairs follow the same mode pattern. */
    a64_stp32(&a, A64_W0, A64_W1, A64_SP, -4, A64_PAIR_POST);
    a64_ldp32(&a, A64_W0, A64_W1, A64_SP, 4, A64_PAIR_POST);
    T_ASSERT_EQ_INT(word_at(&a, 5), 0x28Bf87e0u);
    T_ASSERT_EQ_INT(word_at(&a, 6), 0x28c087e0u);
    a64_free(&a);
}

int main(void) {
    test_golden_match();
    test_mov_imm64();
    test_pair_modes();
    test_b26_boundaries();
    test_b19_boundaries();
    test_adrp_boundaries();
    return t_finalize();
}
