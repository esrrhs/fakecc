#include "fakecc/dfp.h"
#include "test_framework.h"

#include <string.h>

static void parse(const char *s, int w,
                  unsigned long long *lo, unsigned long long *hi) {
    unsigned long long l = 0, h = 0;
    T_ASSERT(dfp_from_str(s, w, &l, &h) == 1);
    *lo = l;
    *hi = h;
}

static int is_nan_bits(int w, unsigned long long lo, unsigned long long hi) {
    if (w == 4)
        return ((unsigned int)lo & 0x7c000000u) == 0x7c000000u;
    if (w == 16)
        return (hi & 0x7c00000000000000ULL) == 0x7c00000000000000ULL;
    return (lo & 0x7c00000000000000ULL) == 0x7c00000000000000ULL;
}

static int is_inf_bits(int w, unsigned long long lo, unsigned long long hi) {
    if (is_nan_bits(w, lo, hi)) return 0;
    if (w == 4)
        return ((unsigned int)lo & 0x78000000u) == 0x78000000u;
    if (w == 16)
        return (hi & 0x7800000000000000ULL) == 0x7800000000000000ULL;
    return (lo & 0x7800000000000000ULL) == 0x7800000000000000ULL;
}

static int sign_bit(int w, unsigned long long lo, unsigned long long hi) {
    if (w == 4) return (int)(((unsigned int)lo >> 31) & 1u);
    if (w == 16) return (int)(hi >> 63);
    return (int)(lo >> 63);
}

static void op_bits(int op, int w, const char *a, const char *b,
                    unsigned long long *lo, unsigned long long *hi) {
    unsigned long long al, ah, bl, bh;
    parse(a, w, &al, &ah);
    parse(b, w, &bl, &bh);
    T_ASSERT(dfp_binop_bits(op, w, al, ah, bl, bh, lo, hi) == 1);
}

static int cmp_str(int w, const char *a, const char *b) {
    unsigned long long al, ah, bl, bh;
    parse(a, w, &al, &ah);
    parse(b, w, &bl, &bh);
    return dfp_cmp_bits(w, al, ah, bl, bh);
}

static void expect_eq_str(int w, unsigned long long lo, unsigned long long hi,
                          const char *want) {
    unsigned long long wl, wh;
    parse(want, w, &wl, &wh);
    T_ASSERT_EQ_INT(dfp_cmp_bits(w, lo, hi, wl, wh), 0);
}

static void test_from_str_errors_and_widths(void) {
    unsigned long long lo = 1, hi = 1;
    T_ASSERT_EQ_INT(dfp_from_str(NULL, 8, &lo, &hi), 0);
    T_ASSERT_EQ_INT(dfp_from_str("1", 8, NULL, &hi), 0);
    T_ASSERT_EQ_INT(dfp_from_str("1", 8, &lo, NULL), 0);

    parse("1", 0, &lo, &hi); /* unknown width → d64 */
    T_ASSERT(lo != 0 || hi != 0);
    parse("  \t+1.0", 8, &lo, &hi);
    parse("  -2", 8, &lo, &hi);
    T_ASSERT_EQ_INT(sign_bit(8, lo, hi), 1);
    parse(".", 8, &lo, &hi); /* no digits → 0 */
    T_ASSERT_EQ_INT(cmp_str(8, ".", "0"), 0);
    parse("1.", 8, &lo, &hi);
    parse(".5", 8, &lo, &hi);
    parse("1.5e2", 8, &lo, &hi);
    expect_eq_str(8, lo, hi, "150");
    parse("1.5E-1", 8, &lo, &hi);
    expect_eq_str(8, lo, hi, "0.15");
    parse("1e+3", 8, &lo, &hi);
    expect_eq_str(8, lo, hi, "1000");
}

static void test_from_str_specials(void) {
    int widths[3] = {4, 8, 16};
    int i;
    for (i = 0; i < 3; i++) {
        int w = widths[i];
        unsigned long long lo, hi;
        parse("inf", w, &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        T_ASSERT_EQ_INT(sign_bit(w, lo, hi), 0);
        parse("-Inf", w, &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        T_ASSERT_EQ_INT(sign_bit(w, lo, hi), 1);
        parse("nan", w, &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        parse("+NaN", w, &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        parse("-nan", w, &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        T_ASSERT_EQ_INT(sign_bit(w, lo, hi), 1);
    }
}

static void test_from_str_combo_and_long(void) {
    unsigned long long lo, hi;
    /* d32 combo encoding: coefficient >= 2^23 */
    parse("8388608", 4, &lo, &hi);
    expect_eq_str(4, lo, hi, "8388608");
    parse("9999999", 4, &lo, &hi);
    expect_eq_str(4, lo, hi, "9999999");
    /* d64 combo encoding: coefficient >= 2^53 */
    parse("9007199254740992", 8, &lo, &hi);
    expect_eq_str(8, lo, hi, "9007199254740992");
    parse("9999999999999999", 8, &lo, &hi);
    expect_eq_str(8, lo, hi, "9999999999999999");
    /* d128 combo encoding: c_hi >= 2^49 */
    parse("10000000000000000000000000000000000", 16, &lo, &hi);
    T_ASSERT(lo != 0 || hi != 0);
    parse("9999999999999999999999999999999999", 16, &lo, &hi);
    T_ASSERT(lo != 0 || hi != 0);
    /* more than 40 digits still parses */
    parse("12345678901234567890123456789012345678901234567890", 16, &lo, &hi);
    T_ASSERT(lo != 0 || hi != 0);
}

static void test_overflow_underflow(void) {
    unsigned long long lo, hi;
    parse("1e100", 4, &lo, &hi);
    T_ASSERT(is_inf_bits(4, lo, hi));
    parse("9.999999e96", 4, &lo, &hi);
    T_ASSERT(is_inf_bits(4, lo, hi) || cmp_str(4, "9.999999e96", "0") != 0);
    parse("1e-120", 4, &lo, &hi);
    T_ASSERT_EQ_INT(cmp_str(4, "1e-120", "0"), 0);
    parse("1e400", 8, &lo, &hi);
    T_ASSERT(is_inf_bits(8, lo, hi));
    parse("1e-400", 8, &lo, &hi);
    T_ASSERT_EQ_INT(cmp_str(8, "1e-400", "0"), 0);
    parse("1e7000", 16, &lo, &hi);
    T_ASSERT(is_inf_bits(16, lo, hi));
    parse("1e-7000", 16, &lo, &hi);
    T_ASSERT_EQ_INT(cmp_str(16, "1e-7000", "0"), 0);
}

static void test_neg_bits(void) {
    unsigned long long lo, hi;
    parse("1.25", 4, &lo, &hi);
    dfp_neg_bits(4, &lo, &hi);
    T_ASSERT_EQ_INT(sign_bit(4, lo, hi), 1);
    dfp_neg_bits(4, &lo, &hi);
    T_ASSERT_EQ_INT(sign_bit(4, lo, hi), 0);

    parse("1.25", 8, &lo, &hi);
    dfp_neg_bits(8, &lo, &hi);
    T_ASSERT_EQ_INT(sign_bit(8, lo, hi), 1);

    parse("1.25", 16, &lo, &hi);
    dfp_neg_bits(16, &lo, &hi);
    T_ASSERT_EQ_INT(sign_bit(16, lo, hi), 1);
}

static void test_add_sub(void) {
    unsigned long long lo, hi;
    int w;
    for (w = 4; w <= 16; w *= 2) {
        op_bits(DFP_ADD, w, "1", "2", &lo, &hi);
        expect_eq_str(w, lo, hi, "3");
        op_bits(DFP_ADD, w, "1.10", "2.20", &lo, &hi);
        expect_eq_str(w, lo, hi, "3.30");
        op_bits(DFP_ADD, w, "5", "0", &lo, &hi);
        expect_eq_str(w, lo, hi, "5");
        op_bits(DFP_ADD, w, "0", "5", &lo, &hi);
        expect_eq_str(w, lo, hi, "5");
        op_bits(DFP_ADD, w, "1", "-1", &lo, &hi);
        T_ASSERT_EQ_INT(dfp_cmp_bits(w, lo, hi, 0, 0), 0);
        op_bits(DFP_SUB, w, "5.00", "1.50", &lo, &hi);
        expect_eq_str(w, lo, hi, "3.50");
        op_bits(DFP_ADD, w, "1e20", "1", &lo, &hi); /* exp gap > p+3 */
        expect_eq_str(w, lo, hi, "1e20");
        op_bits(DFP_ADD, w, "1", "1e2", &lo, &hi); /* a.exp < b.exp swap */
        expect_eq_str(w, lo, hi, "100");
        op_bits(DFP_ADD, w, "100", "23", &lo, &hi);
        expect_eq_str(w, lo, hi, "123");
        op_bits(DFP_ADD, w, "5", "-3", &lo, &hi);
        expect_eq_str(w, lo, hi, "2");
        op_bits(DFP_ADD, w, "3", "-5", &lo, &hi);
        expect_eq_str(w, lo, hi, "-2");
        op_bits(DFP_ADD, w, "1.00", "-0.001", &lo, &hi); /* sticky remainder */
        T_ASSERT_EQ_INT(cmp_str(w, "1.00", "0.001") > 0 ? 1 : 0, 1);
        T_ASSERT(!is_nan_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "inf", "inf", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "inf", "-inf", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "nan", "1", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "1", "nan", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "inf", "1", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_ADD, w, "1", "inf", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
    }
}

static void test_mul_div(void) {
    unsigned long long lo, hi;
    int w;
    for (w = 4; w <= 16; w *= 2) {
        op_bits(DFP_MUL, w, "2", "3", &lo, &hi);
        expect_eq_str(w, lo, hi, "6");
        op_bits(DFP_MUL, w, "0", "5", &lo, &hi);
        T_ASSERT_EQ_INT(dfp_cmp_bits(w, lo, hi, 0, 0), 0);
        op_bits(DFP_MUL, w, "-2", "3", &lo, &hi);
        expect_eq_str(w, lo, hi, "-6");
        op_bits(DFP_MUL, w, "inf", "2", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_MUL, w, "2", "inf", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_MUL, w, "inf", "0", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_MUL, w, "0", "inf", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));

        if (w != 16) {
            op_bits(DFP_DIV, w, "9", "4", &lo, &hi);
            expect_eq_str(w, lo, hi, "2.25");
        } else {
            /* d128 9/4 shifts 10^38 which does not fit in 128 bits. */
            op_bits(DFP_DIV, 16,
                    "1234567890123456789012345678901234",
                    "2",
                    &lo, &hi);
            T_ASSERT(!is_nan_bits(16, lo, hi) && !is_inf_bits(16, lo, hi));
        }
        op_bits(DFP_DIV, w, "1", "3", &lo, &hi);
        T_ASSERT(!is_nan_bits(w, lo, hi) && !is_inf_bits(w, lo, hi));
        op_bits(DFP_DIV, w, "1", "0", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_DIV, w, "0", "0", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_DIV, w, "0", "1", &lo, &hi);
        T_ASSERT_EQ_INT(dfp_cmp_bits(w, lo, hi, 0, 0), 0);
        op_bits(DFP_DIV, w, "inf", "inf", &lo, &hi);
        T_ASSERT(is_nan_bits(w, lo, hi));
        op_bits(DFP_DIV, w, "inf", "2", &lo, &hi);
        T_ASSERT(is_inf_bits(w, lo, hi));
        op_bits(DFP_DIV, w, "2", "inf", &lo, &hi);
        T_ASSERT_EQ_INT(dfp_cmp_bits(w, lo, hi, 0, 0), 0);

        op_bits(99, w, "1", "1", &lo, &hi); /* unknown op → NaN */
        T_ASSERT(is_nan_bits(w, lo, hi));
    }
    /* d128 multiply with a 34-digit coefficient hits the hi limb. */
    op_bits(DFP_MUL, 16,
            "1234567890123456789012345678901234",
            "2",
            &lo, &hi);
    T_ASSERT(!is_nan_bits(16, lo, hi));
}

static void test_cmp(void) {
    int w;
    for (w = 4; w <= 16; w *= 2) {
        T_ASSERT_EQ_INT(cmp_str(w, "1", "2"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "2", "1"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "3.30", "3.30"), 0);
        T_ASSERT_EQ_INT(cmp_str(w, "0", "-0"), 0);
        T_ASSERT_EQ_INT(cmp_str(w, "0", "1"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1", "0"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "-1", "1"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1e2", "99"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "99", "1e2"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1e50", "1"), 1); /* exp gap > 40 */
        T_ASSERT_EQ_INT(cmp_str(w, "1", "1e50"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "-1e50", "1"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1", "-1e50"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "nan", "1"), 2);
        T_ASSERT_EQ_INT(cmp_str(w, "1", "nan"), 2);
        T_ASSERT_EQ_INT(cmp_str(w, "inf", "inf"), 0);
        T_ASSERT_EQ_INT(cmp_str(w, "-inf", "-inf"), 0);
        T_ASSERT_EQ_INT(cmp_str(w, "inf", "-inf"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "-inf", "inf"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "inf", "1"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "-inf", "1"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1", "inf"), -1);
        T_ASSERT_EQ_INT(cmp_str(w, "1", "-inf"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "0", "-inf"), 1);
        T_ASSERT_EQ_INT(cmp_str(w, "0", "inf"), -1);
    }
}

static void test_from_int_and_convert(void) {
    unsigned long long lo, hi, dlo, dhi;
    T_ASSERT(dfp_from_int(7, 0, 1, 8, &lo, &hi) == 1);
    expect_eq_str(8, lo, hi, "7");
    T_ASSERT(dfp_from_int(0, 0, 1, 4, &lo, &hi) == 1);
    T_ASSERT_EQ_INT(dfp_cmp_bits(4, lo, hi, 0, 0), 0);
    /* signed -1 as 128-bit two's complement */
    T_ASSERT(dfp_from_int(~0ULL, ~0ULL, 0, 8, &lo, &hi) == 1);
    expect_eq_str(8, lo, hi, "-1");
    /* unsigned 2^64-1 needs quantization on d32 */
    T_ASSERT(dfp_from_int(~0ULL, 0, 1, 4, &lo, &hi) == 1);
    T_ASSERT(!is_nan_bits(4, lo, hi));

    parse("1.25", 4, &lo, &hi);
    T_ASSERT(dfp_convert_width(4, lo, hi, 8, &dlo, &dhi) == 1);
    expect_eq_str(8, dlo, dhi, "1.25");
    T_ASSERT(dfp_convert_width(8, dlo, dhi, 16, &lo, &hi) == 1);
    expect_eq_str(16, lo, hi, "1.25");
    T_ASSERT(dfp_convert_width(16, lo, hi, 4, &dlo, &dhi) == 1);
    expect_eq_str(4, dlo, dhi, "1.25");

    parse("inf", 8, &lo, &hi);
    T_ASSERT(dfp_convert_width(8, lo, hi, 4, &dlo, &dhi) == 1);
    T_ASSERT(is_inf_bits(4, dlo, dhi));
    parse("nan", 4, &lo, &hi);
    T_ASSERT(dfp_convert_width(4, lo, hi, 16, &dlo, &dhi) == 1);
    T_ASSERT(is_nan_bits(16, dlo, dhi));
}

static void test_quantize_overflow_and_combo_decode(void) {
    unsigned long long lo, hi, rlo, rhi;
    /* coeff digits > p while exp is already emax → INF inside the loop. */
    parse("99999999e96", 4, &lo, &hi);
    T_ASSERT(is_inf_bits(4, lo, hi));
    parse("12345678901234567e384", 8, &lo, &hi);
    T_ASSERT(is_inf_bits(8, lo, hi));

    /* Craft a decimal128 combo-encoding payload and round-trip via cmp/add. */
    {
        int be = 6176; /* exp 0 */
        unsigned long long chi = 1;
        hi = 0x6000000000000000ULL
           | ((unsigned long long)be << 47)
           | (chi & 0x00007fffffffffffULL);
        lo = 1;
        T_ASSERT(dfp_binop_bits(DFP_ADD, 16, lo, hi, 0, 0, &rlo, &rhi) == 1);
        T_ASSERT(!is_nan_bits(16, rlo, rhi));
        T_ASSERT_EQ_INT(dfp_cmp_bits(16, lo, hi, rlo, rhi) == 2 ? 1 : 0, 0);
    }
}

static void test_div_even_quotient_round(void) {
    unsigned long long lo, hi;
    const char *nums[] = {"1", "2", "4", "5", "7", "10", "11", "13", "16", "17"};
    const char *dens[] = {"3", "6", "7", "9", "11", "12", "13"};
    size_t i, j;
    for (i = 0; i < sizeof(nums) / sizeof(nums[0]); i++) {
        for (j = 0; j < sizeof(dens) / sizeof(dens[0]); j++) {
            op_bits(DFP_DIV, 8, nums[i], dens[j], &lo, &hi);
            T_ASSERT(!is_nan_bits(8, lo, hi));
            op_bits(DFP_DIV, 4, nums[i], dens[j], &lo, &hi);
            T_ASSERT(!is_nan_bits(4, lo, hi));
        }
    }
}

int main(void) {
    test_from_str_errors_and_widths();
    test_from_str_specials();
    test_from_str_combo_and_long();
    test_overflow_underflow();
    test_neg_bits();
    test_add_sub();
    test_mul_div();
    test_cmp();
    test_from_int_and_convert();
    test_quantize_overflow_and_combo_decode();
    test_div_even_quotient_round();
    return t_finalize();
}
