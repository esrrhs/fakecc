/* IEEE 754 BID decimal32/64/128 arithmetic for FakeCC (no preprocessor). */
package runtime;

enum {
    DFP_FINITE = 0,
    DFP_INF = 1,
    DFP_NAN = 2
};

struct Dfp {
    int cls;
    int sign;
    int exp;
    unsigned long long c_lo;
    unsigned long long c_hi;
};

static unsigned long long dfp_p10_lo(int n) {
    unsigned long long r = 1;
    int i = 0;
    if (n <= 0) return 1;
    if (n > 19) n = 19;
    while (i < n) {
        r = r * 10ULL;
        i = i + 1;
    }
    return r;
}

static void u128_add(unsigned long long *lo, unsigned long long *hi,
                     unsigned long long blo, unsigned long long bhi) {
    unsigned long long nlo = *lo + blo;
    unsigned long long c = (nlo < *lo) ? 1ULL : 0ULL;
    *lo = nlo;
    *hi = *hi + bhi + c;
}

static void u128_sub(unsigned long long *lo, unsigned long long *hi,
                     unsigned long long blo, unsigned long long bhi) {
    unsigned long long br = (*lo < blo) ? 1ULL : 0ULL;
    *lo = *lo - blo;
    *hi = *hi - bhi - br;
}

static int u128_ge(unsigned long long alo, unsigned long long ahi,
                   unsigned long long blo, unsigned long long bhi) {
    if (ahi != bhi) return ahi > bhi;
    return alo >= blo;
}

static void mul64(unsigned long long a, unsigned long long b,
                  unsigned long long *lo, unsigned long long *hi) {
    unsigned long long a0 = a & 0xffffffffULL;
    unsigned long long a1 = a >> 32;
    unsigned long long b0 = b & 0xffffffffULL;
    unsigned long long b1 = b >> 32;
    unsigned long long p0 = a0 * b0;
    unsigned long long p1 = a0 * b1;
    unsigned long long p2 = a1 * b0;
    unsigned long long p3 = a1 * b1;
    unsigned long long mid = (p0 >> 32) + (p1 & 0xffffffffULL) + (p2 & 0xffffffffULL);
    *lo = (p0 & 0xffffffffULL) | (mid << 32);
    *hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
}

static void u128_mul_u64(unsigned long long *lo, unsigned long long *hi,
                         unsigned long long m) {
    unsigned long long p0l, p0h, p1l, p1h;
    mul64(*lo, m, &p0l, &p0h);
    mul64(*hi, m, &p1l, &p1h);
    *lo = p0l;
    *hi = p0h + p1l;
    /* p1h discarded if overflow past 128 — caller must keep coeff in range */
    (void)p1h;
}

static void u128_div10(unsigned long long *lo, unsigned long long *hi,
                       unsigned long long *rem) {
    unsigned int w0 = (unsigned int)(*lo);
    unsigned int w1 = (unsigned int)(*lo >> 32);
    unsigned int w2 = (unsigned int)(*hi);
    unsigned int w3 = (unsigned int)(*hi >> 32);
    unsigned long long r = 0;
    unsigned long long cur;
    unsigned int q3, q2, q1, q0;
    cur = (r << 32) | w3; q3 = (unsigned int)(cur / 10ULL); r = cur % 10ULL;
    cur = (r << 32) | w2; q2 = (unsigned int)(cur / 10ULL); r = cur % 10ULL;
    cur = (r << 32) | w1; q1 = (unsigned int)(cur / 10ULL); r = cur % 10ULL;
    cur = (r << 32) | w0; q0 = (unsigned int)(cur / 10ULL); r = cur % 10ULL;
    *lo = ((unsigned long long)q1 << 32) | q0;
    *hi = ((unsigned long long)q3 << 32) | q2;
    *rem = r;
}

static int dfp_p(int w) {
    if (w == 4) return 7;
    if (w == 16) return 34;
    return 16;
}
static int dfp_bias(int w) {
    if (w == 4) return 101;
    if (w == 16) return 6176;
    return 398;
}
static int dfp_emax(int w) {
    if (w == 4) return 96;
    if (w == 16) return 6144;
    return 384;
}
static int dfp_emin(int w) {
    if (w == 4) return -95;
    if (w == 16) return -6143;
    return -383;
}

static int dfp_ndigits(unsigned long long lo, unsigned long long hi) {
    if (lo == 0 && hi == 0) return 1;
    int n = 1;
    unsigned long long tlo = 10;
    unsigned long long thi = 0;
    while (u128_ge(lo, hi, tlo, thi)) {
        n = n + 1;
        if (n >= 40) return n;
        u128_mul_u64(&tlo, &thi, 10ULL);
        if (thi > 0x0fffffffffffffffULL && tlo > 0) return n + 1;
    }
    return n;
}

static void dfp_quantize(struct Dfp *x, int width) {
    int p = dfp_p(width);
    int emax = dfp_emax(width);
    int emin = dfp_emin(width);
    int etiny = emin - (p - 1);
    unsigned long long rem;
    if (x->cls != DFP_FINITE) return;
    if (x->c_lo == 0 && x->c_hi == 0) {
        x->exp = 0;
        return;
    }
    while (dfp_ndigits(x->c_lo, x->c_hi) > p || x->exp < etiny) {
        if (x->exp >= emax && dfp_ndigits(x->c_lo, x->c_hi) > p) {
            x->cls = DFP_INF;
            x->c_lo = 0;
            x->c_hi = 0;
            x->exp = 0;
            return;
        }
        u128_div10(&x->c_lo, &x->c_hi, &rem);
        if (rem > 5ULL || (rem == 5ULL && (x->c_lo & 1ULL)))
            u128_add(&x->c_lo, &x->c_hi, 1ULL, 0ULL);
        x->exp = x->exp + 1;
        if (x->c_lo == 0 && x->c_hi == 0) {
            x->exp = 0;
            return;
        }
    }
    if (x->exp > emax) {
        x->cls = DFP_INF;
        x->c_lo = 0;
        x->c_hi = 0;
        x->exp = 0;
    }
}

static void dfp_encode(struct Dfp *x, int width,
                       unsigned long long *lo, unsigned long long *hi) {
    *lo = 0;
    *hi = 0;
    if (width == 4) {
        unsigned int bits = 0;
        if (x->sign) bits = bits | 0x80000000u;
        if (x->cls == DFP_NAN) bits = bits | 0x7c000000u;
        else if (x->cls == DFP_INF) bits = bits | 0x78000000u;
        else {
            int be = x->exp + dfp_bias(4);
            unsigned int c = (unsigned int)x->c_lo;
            if (c < (1u << 23))
                bits = bits | ((unsigned int)be << 23) | c;
            else
                bits = bits | 0x60000000u | ((unsigned int)be << 21) | (c & 0x1fffffu);
        }
        *lo = bits;
        return;
    }
    if (width != 16) {
        unsigned long long bits = 0;
        if (x->sign) bits = bits | 0x8000000000000000ULL;
        if (x->cls == DFP_NAN) bits = bits | 0x7c00000000000000ULL;
        else if (x->cls == DFP_INF) bits = bits | 0x7800000000000000ULL;
        else {
            int be = x->exp + dfp_bias(8);
            unsigned long long c = x->c_lo;
            if (c < (1ULL << 53))
                bits = bits | ((unsigned long long)be << 53) | c;
            else
                bits = bits | 0x6000000000000000ULL
                            | ((unsigned long long)be << 51)
                            | (c & 0x0007ffffffffffffULL);
        }
        *lo = bits;
        return;
    }
    if (x->sign) *hi = *hi | 0x8000000000000000ULL;
    if (x->cls == DFP_NAN) *hi = *hi | 0x7c00000000000000ULL;
    else if (x->cls == DFP_INF) *hi = *hi | 0x7800000000000000ULL;
    else {
        int be = x->exp + dfp_bias(16);
        unsigned long long th = 0x0002000000000000ULL;
        if (x->c_hi < th) {
            *hi = *hi | ((unsigned long long)be << 49) | x->c_hi;
            *lo = x->c_lo;
        } else {
            unsigned long long clo = x->c_lo;
            unsigned long long chi = x->c_hi;
            u128_sub(&clo, &chi, 0ULL, th);
            *hi = *hi | 0x6000000000000000ULL
                      | ((unsigned long long)be << 47)
                      | (chi & 0x00007fffffffffffULL);
            *lo = clo;
        }
    }
}

static void dfp_decode(int width, unsigned long long lo, unsigned long long hi,
                       struct Dfp *x) {
    x->cls = DFP_FINITE;
    x->sign = 0;
    x->exp = 0;
    x->c_lo = 0;
    x->c_hi = 0;
    if (width == 4) {
        unsigned int bits = (unsigned int)lo;
        x->sign = (bits >> 31) & 1;
        if ((bits & 0x7c000000u) == 0x7c000000u) { x->cls = DFP_NAN; return; }
        if ((bits & 0x78000000u) == 0x78000000u) { x->cls = DFP_INF; return; }
        if ((bits & 0x60000000u) == 0x60000000u) {
            x->exp = (int)((bits >> 21) & 0xff) - dfp_bias(4);
            x->c_lo = (unsigned long long)((bits & 0x1fffffu) | 0x800000u);
        } else {
            x->exp = (int)((bits >> 23) & 0xff) - dfp_bias(4);
            x->c_lo = (unsigned long long)(bits & 0x7fffffu);
        }
        return;
    }
    if (width != 16) {
        x->sign = (int)(lo >> 63);
        if ((lo & 0x7c00000000000000ULL) == 0x7c00000000000000ULL) {
            x->cls = DFP_NAN; return;
        }
        if ((lo & 0x7800000000000000ULL) == 0x7800000000000000ULL) {
            x->cls = DFP_INF; return;
        }
        if ((lo & 0x6000000000000000ULL) == 0x6000000000000000ULL) {
            x->exp = (int)((lo >> 51) & 0x3ff) - dfp_bias(8);
            x->c_lo = (lo & 0x0007ffffffffffffULL) | 0x0020000000000000ULL;
        } else {
            x->exp = (int)((lo >> 53) & 0x3ff) - dfp_bias(8);
            x->c_lo = lo & 0x001fffffffffffffULL;
        }
        return;
    }
    x->sign = (int)(hi >> 63);
    if ((hi & 0x7c00000000000000ULL) == 0x7c00000000000000ULL) {
        x->cls = DFP_NAN; return;
    }
    if ((hi & 0x7800000000000000ULL) == 0x7800000000000000ULL) {
        x->cls = DFP_INF; return;
    }
    if ((hi & 0x6000000000000000ULL) == 0x6000000000000000ULL) {
        x->exp = (int)((hi >> 47) & 0x3fff) - dfp_bias(16);
        x->c_lo = lo;
        x->c_hi = (hi & 0x00007fffffffffffULL) | 0x0002000000000000ULL;
    } else {
        x->exp = (int)((hi >> 49) & 0x3fff) - dfp_bias(16);
        x->c_lo = lo;
        x->c_hi = hi & 0x0001ffffffffffffULL;
    }
}

static void dfp_shift10(struct Dfp *x, int n) {
    int i = 0;
    if (n <= 0) return;
    while (i < n) {
        u128_mul_u64(&x->c_lo, &x->c_hi, 10ULL);
        i = i + 1;
    }
}

static struct Dfp dfp_add_finite(struct Dfp a, struct Dfp b, int width) {
    struct Dfp r;
    r.cls = DFP_FINITE;
    r.sign = 0;
    r.exp = 0;
    r.c_lo = 0;
    r.c_hi = 0;
    if (a.c_lo == 0 && a.c_hi == 0) return b;
    if (b.c_lo == 0 && b.c_hi == 0) return a;
    if (a.exp < b.exp) {
        struct Dfp t = a;
        a = b;
        b = t;
    }
    int shift = a.exp - b.exp;
    int p = dfp_p(width);
    if (shift > p + 3) return a;
    unsigned long long rem = 0;
    unsigned long long sticky = 0;
    int i = 0;
    while (i < shift) {
        u128_div10(&b.c_lo, &b.c_hi, &rem);
        if (rem) sticky = 1;
        i = i + 1;
    }
    r.exp = a.exp;
    if (a.sign == b.sign) {
        r.sign = a.sign;
        r.c_lo = a.c_lo;
        r.c_hi = a.c_hi;
        u128_add(&r.c_lo, &r.c_hi, b.c_lo, b.c_hi);
    } else {
        if (u128_ge(a.c_lo, a.c_hi, b.c_lo, b.c_hi)) {
            r.sign = a.sign;
            r.c_lo = a.c_lo;
            r.c_hi = a.c_hi;
            u128_sub(&r.c_lo, &r.c_hi, b.c_lo, b.c_hi);
            if (sticky && (r.c_lo != 0 || r.c_hi != 0))
                u128_sub(&r.c_lo, &r.c_hi, 1ULL, 0ULL);
            if (r.c_lo == 0 && r.c_hi == 0) r.sign = 0;
        } else {
            r.sign = b.sign;
            r.c_lo = b.c_lo;
            r.c_hi = b.c_hi;
            u128_sub(&r.c_lo, &r.c_hi, a.c_lo, a.c_hi);
        }
    }
    dfp_quantize(&r, width);
    return r;
}

static struct Dfp dfp_mul_finite(struct Dfp a, struct Dfp b, int width) {
    struct Dfp r;
    r.cls = DFP_FINITE;
    r.sign = a.sign ^ b.sign;
    r.exp = a.exp + b.exp;
    r.c_lo = 0;
    r.c_hi = 0;
    if ((a.c_lo == 0 && a.c_hi == 0) || (b.c_lo == 0 && b.c_hi == 0)) {
        r.exp = 0;
        r.sign = a.sign ^ b.sign;
        return r;
    }
    unsigned long long p00l, p00h, p01l, p01h, p10l, p10h;
    mul64(a.c_lo, b.c_lo, &p00l, &p00h);
    mul64(a.c_lo, b.c_hi, &p01l, &p01h);
    mul64(a.c_hi, b.c_lo, &p10l, &p10h);
    r.c_lo = p00l;
    r.c_hi = p00h;
    u128_add(&r.c_hi, &p01h, p01l, 0ULL);
    u128_add(&r.c_hi, &p10h, p10l, 0ULL);
    (void)p01h;
    (void)p10h;
    dfp_quantize(&r, width);
    return r;
}

static struct Dfp dfp_div_finite(struct Dfp a, struct Dfp b, int width) {
    struct Dfp r;
    r.cls = DFP_FINITE;
    r.sign = a.sign ^ b.sign;
    r.exp = 0;
    r.c_lo = 0;
    r.c_hi = 0;
    if (b.c_lo == 0 && b.c_hi == 0) {
        r.cls = (a.c_lo == 0 && a.c_hi == 0) ? DFP_NAN : DFP_INF;
        return r;
    }
    if (a.c_lo == 0 && a.c_hi == 0) return r;
    int p = dfp_p(width);
    int k = p + 4 - dfp_ndigits(a.c_lo, a.c_hi) + dfp_ndigits(b.c_lo, b.c_hi);
    if (k < p + 2) k = p + 2;
    struct Dfp num = a;
    dfp_shift10(&num, k);
    /* restoring division of 128/128 → 128 */
    unsigned long long qlo = 0;
    unsigned long long qhi = 0;
    unsigned long long rlo = 0;
    unsigned long long rhi = 0;
    int bit = 127;
    while (bit >= 0) {
        rhi = (rhi << 1) | (rlo >> 63);
        rlo = rlo << 1;
        unsigned long long nb;
        if (bit >= 64) nb = (num.c_hi >> (bit - 64)) & 1ULL;
        else nb = (num.c_lo >> bit) & 1ULL;
        rlo = rlo | nb;
        if (u128_ge(rlo, rhi, b.c_lo, b.c_hi)) {
            u128_sub(&rlo, &rhi, b.c_lo, b.c_hi);
            if (bit >= 64) qhi = qhi | (1ULL << (bit - 64));
            else qlo = qlo | (1ULL << bit);
        }
        bit = bit - 1;
    }
    if (rlo != 0 || rhi != 0) {
        if ((qlo & 1ULL) == 0)
            u128_add(&qlo, &qhi, 1ULL, 0ULL);
    }
    r.c_lo = qlo;
    r.c_hi = qhi;
    r.exp = a.exp - b.exp - k;
    dfp_quantize(&r, width);
    return r;
}

static struct Dfp dfp_binop(int op, struct Dfp a, struct Dfp b, int width) {
    struct Dfp r;
    r.cls = DFP_NAN;
    r.sign = 0;
    r.exp = 0;
    r.c_lo = 0;
    r.c_hi = 0;
    if (op == 1) {
        b.sign = 1 - b.sign;
        op = 0;
    }
    if (a.cls == DFP_NAN || b.cls == DFP_NAN) {
        r.cls = DFP_NAN;
        r.sign = a.cls == DFP_NAN ? a.sign : b.sign;
        return r;
    }
    if (op == 0) {
        if (a.cls == DFP_INF && b.cls == DFP_INF && a.sign != b.sign) {
            r.cls = DFP_NAN;
            return r;
        }
        if (a.cls == DFP_INF) return a;
        if (b.cls == DFP_INF) return b;
        return dfp_add_finite(a, b, width);
    }
    if (op == 2) {
        r.sign = a.sign ^ b.sign;
        if ((a.cls == DFP_INF && b.cls == DFP_FINITE && b.c_lo == 0 && b.c_hi == 0) ||
            (b.cls == DFP_INF && a.cls == DFP_FINITE && a.c_lo == 0 && a.c_hi == 0)) {
            r.cls = DFP_NAN;
            return r;
        }
        if (a.cls == DFP_INF || b.cls == DFP_INF) {
            r.cls = DFP_INF;
            return r;
        }
        return dfp_mul_finite(a, b, width);
    }
    if (op == 3) {
        r.sign = a.sign ^ b.sign;
        if (a.cls == DFP_INF && b.cls == DFP_INF) {
            r.cls = DFP_NAN;
            return r;
        }
        if (a.cls == DFP_INF) {
            r.cls = DFP_INF;
            return r;
        }
        if (b.cls == DFP_INF) {
            r.cls = DFP_FINITE;
            return r;
        }
        return dfp_div_finite(a, b, width);
    }
    return r;
}

static int dfp_cmp(struct Dfp a, struct Dfp b) {
    if (a.cls == DFP_NAN || b.cls == DFP_NAN) return 2;
    if (a.cls == DFP_INF || b.cls == DFP_INF) {
        if (a.cls == DFP_INF && b.cls == DFP_INF) {
            if (a.sign == b.sign) return 0;
            return a.sign ? -1 : 1;
        }
        if (a.cls == DFP_INF) return a.sign ? -1 : 1;
        return b.sign ? 1 : -1;
    }
    if (a.c_lo == 0 && a.c_hi == 0 && b.c_lo == 0 && b.c_hi == 0) return 0;
    if (a.c_lo == 0 && a.c_hi == 0) return b.sign ? 1 : -1;
    if (b.c_lo == 0 && b.c_hi == 0) return a.sign ? -1 : 1;
    if (a.sign != b.sign) return a.sign ? -1 : 1;
    struct Dfp x = a;
    struct Dfp y = b;
    if (x.exp < y.exp) {
        int s = y.exp - x.exp;
        if (s > 40) return a.sign ? 1 : -1;
        dfp_shift10(&y, s);
    } else if (y.exp < x.exp) {
        int s = x.exp - y.exp;
        if (s > 40) return a.sign ? -1 : 1;
        dfp_shift10(&x, s);
    }
    int mag;
    if (x.c_hi < y.c_hi || (x.c_hi == y.c_hi && x.c_lo < y.c_lo)) mag = -1;
    else if (x.c_hi > y.c_hi || (x.c_hi == y.c_hi && x.c_lo > y.c_lo)) mag = 1;
    else mag = 0;
    if (a.sign) return -mag;
    return mag;
}

static unsigned long long dfp_op64(int op, int width, unsigned long long a, unsigned long long b) {
    struct Dfp da;
    struct Dfp db;
    struct Dfp r;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(width, a, 0, &da);
    dfp_decode(width, b, 0, &db);
    r = dfp_binop(op, da, db, width);
    dfp_encode(&r, width, &lo, &hi);
    return lo;
}

static int dfp_cmp64(int width, unsigned long long a, unsigned long long b) {
    struct Dfp da;
    struct Dfp db;
    dfp_decode(width, a, 0, &da);
    dfp_decode(width, b, 0, &db);
    return dfp_cmp(da, db);
}

unsigned int __bid_addsd3(unsigned int a, unsigned int b) {
    return (unsigned int)dfp_op64(0, 4, a, b);
}
unsigned int __bid_subsd3(unsigned int a, unsigned int b) {
    return (unsigned int)dfp_op64(1, 4, a, b);
}
unsigned int __bid_mulsd3(unsigned int a, unsigned int b) {
    return (unsigned int)dfp_op64(2, 4, a, b);
}
unsigned int __bid_divsd3(unsigned int a, unsigned int b) {
    return (unsigned int)dfp_op64(3, 4, a, b);
}

unsigned long long __bid_adddd3(unsigned long long a, unsigned long long b) {
    return dfp_op64(0, 8, a, b);
}
unsigned long long __bid_subdd3(unsigned long long a, unsigned long long b) {
    return dfp_op64(1, 8, a, b);
}
unsigned long long __bid_muldd3(unsigned long long a, unsigned long long b) {
    return dfp_op64(2, 8, a, b);
}
unsigned long long __bid_divdd3(unsigned long long a, unsigned long long b) {
    return dfp_op64(3, 8, a, b);
}

int __bid_eqsd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }
int __bid_nesd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }
int __bid_gtsd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }
int __bid_gesd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }
int __bid_ltsd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }
int __bid_lesd2(unsigned int a, unsigned int b) { return dfp_cmp64(4, a, b); }

int __bid_eqdd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }
int __bid_nedd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }
int __bid_gtdd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }
int __bid_gedd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }
int __bid_ltdd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }
int __bid_ledd2(unsigned long long a, unsigned long long b) { return dfp_cmp64(8, a, b); }

unsigned long long __bid_extendsddd2(unsigned int a) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(4, a, 0, &x);
    dfp_quantize(&x, 8);
    dfp_encode(&x, 8, &lo, &hi);
    return lo;
}
unsigned int __bid_truncddsd2(unsigned long long a) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(8, a, 0, &x);
    dfp_quantize(&x, 4);
    dfp_encode(&x, 4, &lo, &hi);
    return (unsigned int)lo;
}

unsigned long long __bid_floatsidd(int v) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    x.cls = DFP_FINITE;
    x.sign = 0;
    x.exp = 0;
    x.c_lo = 0;
    x.c_hi = 0;
    if (v < 0) {
        x.sign = 1;
        x.c_lo = (unsigned long long)(0 - (unsigned int)v);
        if (v == (int)0x80000000) x.c_lo = 0x80000000ULL;
    } else {
        x.c_lo = (unsigned long long)v;
    }
    dfp_quantize(&x, 8);
    dfp_encode(&x, 8, &lo, &hi);
    return lo;
}
unsigned int __bid_floatsisd(int v) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    x.cls = DFP_FINITE;
    x.sign = 0;
    x.exp = 0;
    x.c_lo = 0;
    x.c_hi = 0;
    if (v < 0) {
        x.sign = 1;
        x.c_lo = (unsigned long long)(0 - (unsigned int)v);
        if (v == (int)0x80000000) x.c_lo = 0x80000000ULL;
    } else {
        x.c_lo = (unsigned long long)v;
    }
    dfp_quantize(&x, 4);
    dfp_encode(&x, 4, &lo, &hi);
    return (unsigned int)lo;
}

unsigned long long __bid_floatdisdd(long long v) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    x.cls = DFP_FINITE;
    x.sign = 0;
    x.exp = 0;
    x.c_lo = 0;
    x.c_hi = 0;
    if (v < 0) {
        x.sign = 1;
        x.c_lo = 0ULL - (unsigned long long)v;
        if (v == (long long)0x8000000000000000ULL) x.c_lo = 0x8000000000000000ULL;
    } else {
        x.c_lo = (unsigned long long)v;
    }
    dfp_quantize(&x, 8);
    dfp_encode(&x, 8, &lo, &hi);
    return lo;
}

int __bid_fixddsi(unsigned long long a) {
    struct Dfp x;
    dfp_decode(8, a, 0, &x);
    if (x.cls != DFP_FINITE) return 0;
    while (x.exp > 0) {
        u128_mul_u64(&x.c_lo, &x.c_hi, 10ULL);
        x.exp = x.exp - 1;
    }
    while (x.exp < 0) {
        unsigned long long rem;
        u128_div10(&x.c_lo, &x.c_hi, &rem);
        x.exp = x.exp + 1;
    }
    if (x.c_hi != 0 || x.c_lo > 0x7fffffffULL) {
        if (x.sign) return (int)0x80000000;
        return 0x7fffffff;
    }
    int v = (int)x.c_lo;
    if (x.sign) v = -v;
    return v;
}
int __bid_fixsdsi(unsigned int a) {
    return __bid_fixddsi(__bid_extendsddd2(a));
}
long long __bid_fixdddi(unsigned long long a) {
    struct Dfp x;
    dfp_decode(8, a, 0, &x);
    if (x.cls != DFP_FINITE) return 0;
    while (x.exp > 0) {
        u128_mul_u64(&x.c_lo, &x.c_hi, 10ULL);
        x.exp = x.exp - 1;
    }
    while (x.exp < 0) {
        unsigned long long rem;
        u128_div10(&x.c_lo, &x.c_hi, &rem);
        x.exp = x.exp + 1;
    }
    if (x.c_hi != 0) {
        if (x.sign) return (long long)0x8000000000000000ULL;
        return (long long)0x7fffffffffffffffULL;
    }
    long long v = (long long)x.c_lo;
    if (x.sign) v = -v;
    return v;
}

unsigned long long __bid_extendsddd2_u(unsigned int a) { return __bid_extendsddd2(a); }

/* Binary64 <-> decimal64: scale mantissa × 2^e into decimal. */
static unsigned long long dfp_from_ieee64(unsigned long long bits, int dst_w) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    int sign = (int)(bits >> 63);
    int exp2 = (int)((bits >> 52) & 0x7ff);
    unsigned long long frac = bits & 0x000fffffffffffffULL;
    x.cls = DFP_FINITE;
    x.sign = sign;
    x.exp = 0;
    x.c_lo = 0;
    x.c_hi = 0;
    if (exp2 == 0x7ff) {
        x.cls = (frac != 0) ? DFP_NAN : DFP_INF;
        dfp_encode(&x, dst_w, &lo, &hi);
        return lo;
    }
    int e;
    unsigned long long m;
    if (exp2 == 0) {
        if (frac == 0) {
            dfp_encode(&x, dst_w, &lo, &hi);
            return lo;
        }
        m = frac;
        e = -1074;
    } else {
        m = frac | (1ULL << 52);
        e = exp2 - 1075;
    }
    x.c_lo = m;
    while (e > 0) {
        u128_mul_u64(&x.c_lo, &x.c_hi, 2ULL);
        e = e - 1;
        if (dfp_ndigits(x.c_lo, x.c_hi) > 36) {
            unsigned long long rem;
            u128_div10(&x.c_lo, &x.c_hi, &rem);
            x.exp = x.exp + 1;
        }
    }
    while (e < 0) {
        if ((x.c_lo & 1ULL) == 0 && x.c_hi == 0) {
            x.c_lo = x.c_lo >> 1;
        } else if ((x.c_lo & 1ULL) == 0) {
            x.c_lo = (x.c_lo >> 1) | (x.c_hi << 63);
            x.c_hi = x.c_hi >> 1;
        } else {
            u128_mul_u64(&x.c_lo, &x.c_hi, 5ULL);
            x.exp = x.exp - 1;
        }
        e = e + 1;
        if (dfp_ndigits(x.c_lo, x.c_hi) > 36) {
            unsigned long long rem;
            u128_div10(&x.c_lo, &x.c_hi, &rem);
            x.exp = x.exp + 1;
        }
    }
    dfp_quantize(&x, dst_w);
    dfp_encode(&x, dst_w, &lo, &hi);
    return lo;
}

unsigned long long __bid_extenddfdd(unsigned long long ieee) {
    return dfp_from_ieee64(ieee, 8);
}
unsigned int __bid_truncdfsd(unsigned long long ieee) {
    return (unsigned int)dfp_from_ieee64(ieee, 4);
}

static unsigned long long dfp_to_ieee64(int width, unsigned long long bits) {
    struct Dfp x;
    dfp_decode(width, bits, 0, &x);
    union { double d; unsigned long long u; } conv;
    if (x.cls == DFP_NAN) {
        conv.u = 0x7ff8000000000000ULL;
        if (x.sign) conv.u = conv.u | 0x8000000000000000ULL;
        return conv.u;
    }
    if (x.cls == DFP_INF) {
        conv.u = 0x7ff0000000000000ULL;
        if (x.sign) conv.u = conv.u | 0x8000000000000000ULL;
        return conv.u;
    }
    if (x.c_lo == 0 && x.c_hi == 0) {
        conv.u = x.sign ? 0x8000000000000000ULL : 0ULL;
        return conv.u;
    }
    /* Approximate: value ≈ coeff * 10^exp.  Convert via repeated *10 /2. */
    double d = (double)x.c_lo;
    if (x.c_hi) d = d + (double)x.c_hi * 18446744073709551616.0;
    int e = x.exp;
    while (e > 0) { d = d * 10.0; e = e - 1; }
    while (e < 0) { d = d / 10.0; e = e + 1; }
    if (x.sign) d = -d;
    conv.d = d;
    return conv.u;
}

unsigned long long __bid_truncdddf(unsigned long long a) {
    return dfp_to_ieee64(8, a);
}
unsigned long long __bid_extendsddf(unsigned int a) {
    return dfp_to_ieee64(4, a);
}

void __fakecc_addtd3(unsigned long long *dlo, unsigned long long *dhi,
                     unsigned long long alo, unsigned long long ahi,
                     unsigned long long blo, unsigned long long bhi) {
    struct Dfp a;
    struct Dfp b;
    struct Dfp r;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(16, alo, ahi, &a);
    dfp_decode(16, blo, bhi, &b);
    r = dfp_binop(0, a, b, 16);
    dfp_encode(&r, 16, &lo, &hi);
    *dlo = lo;
    *dhi = hi;
}
void __fakecc_subtd3(unsigned long long *dlo, unsigned long long *dhi,
                     unsigned long long alo, unsigned long long ahi,
                     unsigned long long blo, unsigned long long bhi) {
    struct Dfp a;
    struct Dfp b;
    struct Dfp r;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(16, alo, ahi, &a);
    dfp_decode(16, blo, bhi, &b);
    r = dfp_binop(1, a, b, 16);
    dfp_encode(&r, 16, &lo, &hi);
    *dlo = lo;
    *dhi = hi;
}
void __fakecc_multd3(unsigned long long *dlo, unsigned long long *dhi,
                     unsigned long long alo, unsigned long long ahi,
                     unsigned long long blo, unsigned long long bhi) {
    struct Dfp a;
    struct Dfp b;
    struct Dfp r;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(16, alo, ahi, &a);
    dfp_decode(16, blo, bhi, &b);
    r = dfp_binop(2, a, b, 16);
    dfp_encode(&r, 16, &lo, &hi);
    *dlo = lo;
    *dhi = hi;
}
void __fakecc_divtd3(unsigned long long *dlo, unsigned long long *dhi,
                     unsigned long long alo, unsigned long long ahi,
                     unsigned long long blo, unsigned long long bhi) {
    struct Dfp a;
    struct Dfp b;
    struct Dfp r;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(16, alo, ahi, &a);
    dfp_decode(16, blo, bhi, &b);
    r = dfp_binop(3, a, b, 16);
    dfp_encode(&r, 16, &lo, &hi);
    *dlo = lo;
    *dhi = hi;
}
int __fakecc_cmptd2(unsigned long long alo, unsigned long long ahi,
                    unsigned long long blo, unsigned long long bhi) {
    struct Dfp a;
    struct Dfp b;
    dfp_decode(16, alo, ahi, &a);
    dfp_decode(16, blo, bhi, &b);
    return dfp_cmp(a, b);
}

unsigned long long __bid_extendddtd2_lo(unsigned long long a) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(8, a, 0, &x);
    dfp_quantize(&x, 16);
    dfp_encode(&x, 16, &lo, &hi);
    return lo;
}
unsigned long long __bid_extendddtd2_hi(unsigned long long a) {
    struct Dfp x;
    unsigned long long lo = 0;
    unsigned long long hi = 0;
    dfp_decode(8, a, 0, &x);
    dfp_quantize(&x, 16);
    dfp_encode(&x, 16, &lo, &hi);
    return hi;
}
void __fakecc_extendddtd2(unsigned long long *dlo, unsigned long long *dhi,
                          unsigned long long a) {
    *dlo = __bid_extendddtd2_lo(a);
    *dhi = __bid_extendddtd2_hi(a);
}
unsigned long long __bid_trunctddd2(unsigned long long lo, unsigned long long hi) {
    struct Dfp x;
    unsigned long long olo = 0;
    unsigned long long ohi = 0;
    dfp_decode(16, lo, hi, &x);
    dfp_quantize(&x, 8);
    dfp_encode(&x, 8, &olo, &ohi);
    return olo;
}
void __fakecc_floatsitd(unsigned long long *dlo, unsigned long long *dhi, int v) {
    unsigned long long d64 = __bid_floatsidd(v);
    __fakecc_extendddtd2(dlo, dhi, d64);
}
int __fakecc_fixtdsi(unsigned long long lo, unsigned long long hi) {
    return __bid_fixddsi(__bid_trunctddd2(lo, hi));
}

/* d32/d64/d128 extend/trunc remaining combinations. */
unsigned int __bid_truncsdsd(unsigned int a) { return a; }
unsigned long long __bid_extendsdtd2_via64(unsigned int a) {
    return __bid_extendsddd2(a);
}
void __fakecc_extendsdtd2(unsigned long long *dlo, unsigned long long *dhi,
                          unsigned int a) {
    __fakecc_extendddtd2(dlo, dhi, __bid_extendsddd2(a));
}
unsigned int __bid_trunctdsd2(unsigned long long lo, unsigned long long hi) {
    return __bid_truncddsd2(__bid_trunctddd2(lo, hi));
}
