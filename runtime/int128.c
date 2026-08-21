/* 128-bit helpers for scalar __int128 (no __int128 in this file).
 * Div/mod plus IEEE conversions matching libgcc __float*ti* / __fix*ti. */
package runtime;

static int u128_ge(unsigned long long alo, unsigned long long ahi,
                   unsigned long long blo, unsigned long long bhi) {
    if (ahi != bhi) return ahi > bhi;
    return alo >= blo;
}

static void u128_sub(unsigned long long *alo, unsigned long long *ahi,
                     unsigned long long blo, unsigned long long bhi) {
    unsigned long long lo = *alo - blo;
    unsigned long long br = (*alo < blo) ? 1ULL : 0ULL;
    *alo = lo;
    *ahi = *ahi - bhi - br;
}

void __fakecc_udivmodti4(unsigned long long nlo, unsigned long long nhi,
                         unsigned long long dlo, unsigned long long dhi,
                         unsigned long long *qlo, unsigned long long *qhi,
                         unsigned long long *rlo, unsigned long long *rhi) {
    unsigned long long q_lo = 0;
    unsigned long long q_hi = 0;
    unsigned long long r_lo = 0;
    unsigned long long r_hi = 0;
    int i = 127;
    if (dlo == 0 && dhi == 0) {
        *qlo = 0;
        *qhi = 0;
        *rlo = nlo;
        *rhi = nhi;
        return;
    }
    while (i >= 0) {
        r_hi = (r_hi << 1) | (r_lo >> 63);
        r_lo = r_lo << 1;
        unsigned long long bit;
        if (i >= 64) bit = (nhi >> (i - 64)) & 1ULL;
        else bit = (nlo >> i) & 1ULL;
        r_lo = r_lo | bit;
        if (u128_ge(r_lo, r_hi, dlo, dhi)) {
            u128_sub(&r_lo, &r_hi, dlo, dhi);
            if (i >= 64) q_hi = q_hi | (1ULL << (i - 64));
            else q_lo = q_lo | (1ULL << i);
        }
        i = i - 1;
    }
    *qlo = q_lo;
    *qhi = q_hi;
    *rlo = r_lo;
    *rhi = r_hi;
}

static int clz64(unsigned long long x) {
    int n = 0;
    if (x == 0) return 64;
    if (x <= 0x00000000FFFFFFFFULL) { n = n + 32; x = x << 32; }
    if (x <= 0x0000FFFFFFFFFFFFULL) { n = n + 16; x = x << 16; }
    if (x <= 0x00FFFFFFFFFFFFFFULL) { n = n + 8; x = x << 8; }
    if (x <= 0x0FFFFFFFFFFFFFFFULL) { n = n + 4; x = x << 4; }
    if (x <= 0x3FFFFFFFFFFFFFFFULL) { n = n + 2; x = x << 2; }
    if (x <= 0x7FFFFFFFFFFFFFFFULL) n = n + 1;
    return n;
}

static void u128_shl(unsigned long long *lo, unsigned long long *hi, int n) {
    if (n <= 0) return;
    if (n >= 128) { *lo = 0; *hi = 0; return; }
    if (n >= 64) { *hi = *lo << (n - 64); *lo = 0; return; }
    *hi = (*hi << n) | (*lo >> (64 - n));
    *lo = *lo << n;
}

static void u128_shr(unsigned long long *lo, unsigned long long *hi, int n) {
    if (n <= 0) return;
    if (n >= 128) { *lo = 0; *hi = 0; return; }
    if (n >= 64) { *lo = *hi >> (n - 64); *hi = 0; return; }
    *lo = (*lo >> n) | (*hi << (64 - n));
    *hi = *hi >> n;
}

static void u128_add1(unsigned long long *lo, unsigned long long *hi) {
    unsigned long long n = *lo + 1ULL;
    if (n < *lo) *hi = *hi + 1ULL;
    *lo = n;
}

static void u128_neg(unsigned long long *lo, unsigned long long *hi) {
    unsigned long long nlo = 0ULL - *lo;
    unsigned long long nhi = 0ULL - *hi;
    if (*lo != 0) nhi = nhi - 1ULL;
    *lo = nlo;
    *hi = nhi;
}

/* compiler-rt __floatXiYf__: round-to-nearest-even into IEEE bits. */
static unsigned long long u128_to_ieee(unsigned long long lo, unsigned long long hi,
                                       int sign, int dst_bits, int dst_sig) {
    int dst_mant = dst_sig + 1;
    unsigned long long signbit;
    unsigned long long fracmask;
    int exp_bits;
    int bias;
    int sd;
    int e;
    if (lo == 0 && hi == 0) return 0;
    if (hi) sd = 128 - clz64(hi);
    else sd = 64 - clz64(lo);
    e = sd - 1;
    if (sd > dst_mant) {
        if (sd == dst_mant + 1) {
            u128_shl(&lo, &hi, 1);
        } else if (sd != dst_mant + 2) {
            int k = sd - (dst_mant + 2);
            unsigned long long slo = lo;
            unsigned long long shi = hi;
            unsigned long long mlo;
            unsigned long long mhi;
            int sticky;
            if (k <= 0) { mlo = 0; mhi = 0; }
            else if (k >= 128) { mlo = ~0ULL; mhi = ~0ULL; }
            else if (k >= 64) { mlo = ~0ULL; mhi = (1ULL << (k - 64)) - 1ULL; }
            else { mlo = (1ULL << k) - 1ULL; mhi = 0; }
            sticky = ((slo & mlo) | (shi & mhi)) != 0;
            u128_shr(&lo, &hi, k);
            if (sticky) lo = lo | 1ULL;
        }
        if (lo & 4ULL) lo = lo | 1ULL;
        u128_add1(&lo, &hi);
        u128_shr(&lo, &hi, 2);
        if (dst_mant >= 64) {
            if (hi & 1ULL) {
                u128_shr(&lo, &hi, 1);
                e = e + 1;
            }
        } else if ((lo >> dst_mant) & 1ULL) {
            u128_shr(&lo, &hi, 1);
            e = e + 1;
        }
    } else {
        u128_shl(&lo, &hi, dst_mant - sd);
    }
    signbit = 1ULL << (dst_bits - 1);
    exp_bits = dst_bits - dst_sig - 1;
    bias = (1 << (exp_bits - 1)) - 1;
    fracmask = (1ULL << dst_sig) - 1ULL;
    return (sign ? signbit : 0ULL)
         | (((unsigned long long)(e + bias)) << dst_sig)
         | (lo & fracmask);
}

static void signed_abs(unsigned long long *lo, unsigned long long *hi, int *sign) {
    if (*hi >> 63) {
        *sign = 1;
        u128_neg(lo, hi);
    } else {
        *sign = 0;
    }
}

static double bits_to_double(unsigned long long bits) {
    double d;
    unsigned char *b = (unsigned char *)&d;
    int i = 0;
    while (i < 8) {
        b[i] = (unsigned char)(bits & 0xffULL);
        bits = bits >> 8;
        i = i + 1;
    }
    return d;
}

static float bits_to_float(unsigned long long bits) {
    float f;
    unsigned char *b = (unsigned char *)&f;
    int i = 0;
    unsigned int u = (unsigned int)bits;
    while (i < 4) {
        b[i] = (unsigned char)(u & 0xffu);
        u = u >> 8;
        i = i + 1;
    }
    return f;
}

static unsigned long long load_bytes(const void *p, int n) {
    const unsigned char *b = (const unsigned char *)p;
    unsigned long long x = 0;
    int i = 0;
    while (i < n) {
        x = x | ((unsigned long long)b[i] << (8 * i));
        i = i + 1;
    }
    return x;
}

double __fakecc_floattidf(unsigned long long lo, unsigned long long hi) {
    int sign;
    signed_abs(&lo, &hi, &sign);
    return bits_to_double(u128_to_ieee(lo, hi, sign, 64, 52));
}

double __fakecc_floatuntidf(unsigned long long lo, unsigned long long hi) {
    return bits_to_double(u128_to_ieee(lo, hi, 0, 64, 52));
}

float __fakecc_floattisf(unsigned long long lo, unsigned long long hi) {
    int sign;
    signed_abs(&lo, &hi, &sign);
    return bits_to_float(u128_to_ieee(lo, hi, sign, 32, 23));
}

float __fakecc_floatuntisf(unsigned long long lo, unsigned long long hi) {
    return bits_to_float(u128_to_ieee(lo, hi, 0, 32, 23));
}

static long double pack_xf(unsigned long long lo, unsigned long long hi, int sign) {
    unsigned char buf[16];
    int i = 0;
    int sd;
    int e;
    unsigned exp;
    long double r;
    while (i < 16) { buf[i] = 0; i = i + 1; }
    if (lo == 0 && hi == 0) {
        r = 0.0L;
        return r;
    }
    if (hi) sd = 128 - clz64(hi);
    else sd = 64 - clz64(lo);
    e = sd - 1;
    if (sd > 64) {
        if (sd == 65) u128_shl(&lo, &hi, 1);
        else if (sd != 66) {
            int k = sd - 66;
            unsigned long long slo = lo, shi = hi, mlo, mhi;
            int sticky;
            if (k <= 0) { mlo = 0; mhi = 0; }
            else if (k >= 128) { mlo = ~0ULL; mhi = ~0ULL; }
            else if (k >= 64) { mlo = ~0ULL; mhi = (1ULL << (k - 64)) - 1ULL; }
            else { mlo = (1ULL << k) - 1ULL; mhi = 0; }
            sticky = ((slo & mlo) | (shi & mhi)) != 0;
            u128_shr(&lo, &hi, k);
            if (sticky) lo = lo | 1ULL;
        }
        if (lo & 4ULL) lo = lo | 1ULL;
        u128_add1(&lo, &hi);
        u128_shr(&lo, &hi, 2);
        if (hi & 1ULL) {
            u128_shr(&lo, &hi, 1);
            e = e + 1;
        }
    } else {
        u128_shl(&lo, &hi, 64 - sd);
    }
    i = 0;
    while (i < 8) {
        buf[i] = (unsigned char)(lo & 0xffULL);
        lo = lo >> 8;
        i = i + 1;
    }
    exp = (unsigned)(e + 16383);
    if (sign) exp = exp | 0x8000u;
    buf[8] = (unsigned char)(exp & 0xffu);
    buf[9] = (unsigned char)((exp >> 8) & 0xffu);
    memcpy(&r, buf, 10);
    return r;
}

long double __fakecc_floattixf(unsigned long long lo, unsigned long long hi) {
    int sign;
    signed_abs(&lo, &hi, &sign);
    return pack_xf(lo, hi, sign);
}

long double __fakecc_floatuntixf(unsigned long long lo, unsigned long long hi) {
    return pack_xf(lo, hi, 0);
}

static void mag_to_i128(int sign, int exp, unsigned long long frac, int frac_bits,
                        int implicit, unsigned long long *olo, unsigned long long *ohi) {
    unsigned long long lo;
    unsigned long long hi;
    int sh;
    if (exp < 0 || exp >= 128) {
        *olo = 0;
        *ohi = 0;
        return;
    }
    lo = frac;
    hi = 0;
    if (implicit) {
        if (frac_bits < 64) lo = lo | (1ULL << frac_bits);
        else hi = hi | (1ULL << (frac_bits - 64));
    }
    sh = exp - frac_bits;
    if (sh < 0) u128_shr(&lo, &hi, -sh);
    else u128_shl(&lo, &hi, sh);
    if (sign) u128_neg(&lo, &hi);
    *olo = lo;
    *ohi = hi;
}

static void nan_pattern(unsigned long long *lo, unsigned long long *hi) {
    *lo = 0x8000000000000000ULL;
    *hi = 0x8000000000000000ULL;
}

static void fix_double(double a, int is_unsigned,
                       unsigned long long *lo, unsigned long long *hi) {
    unsigned long long u = load_bytes(&a, 8);
    int sign = (int)((u >> 63) & 1ULL);
    int expf = (int)((u >> 52) & 0x7ffULL);
    unsigned long long frac = u & 0x000fffffffffffffULL;
    int exp;
    int implicit;
    if (expf == 0x7ff) {
        if (frac == 0) {
            if (sign && is_unsigned) nan_pattern(lo, hi);
            else { *lo = 0; *hi = 0; }
        } else {
            nan_pattern(lo, hi);
        }
        return;
    }
    if (expf == 0) { exp = -1022; implicit = 0; }
    else { exp = expf - 1023; implicit = 1; }
    mag_to_i128(sign, exp, frac, 52, implicit, lo, hi);
}

static void fix_float(float a, int is_unsigned,
                      unsigned long long *lo, unsigned long long *hi) {
    unsigned int u = (unsigned int)load_bytes(&a, 4);
    int sign = (int)((u >> 31) & 1u);
    int expf = (int)((u >> 23) & 0xffu);
    unsigned long long frac = (unsigned long long)(u & 0x7fffffu);
    int exp;
    int implicit;
    if (expf == 0xff) {
        if (frac == 0) {
            if (sign && is_unsigned) nan_pattern(lo, hi);
            else { *lo = 0; *hi = 0; }
        } else {
            nan_pattern(lo, hi);
        }
        return;
    }
    if (expf == 0) { exp = -126; implicit = 0; }
    else { exp = expf - 127; implicit = 1; }
    mag_to_i128(sign, exp, frac, 23, implicit, lo, hi);
}

static void fix_ld(long double a, int is_unsigned,
                   unsigned long long *lo, unsigned long long *hi) {
    unsigned char buf[16];
    unsigned long long frac;
    unsigned expf;
    int sign;
    int exp;
    int i = 0;
    while (i < 16) { buf[i] = 0; i = i + 1; }
    memcpy(buf, &a, 10);
    frac = load_bytes(buf, 8);
    expf = (unsigned)buf[8] | ((unsigned)buf[9] << 8);
    sign = (int)((expf >> 15) & 1u);
    expf = expf & 0x7fffu;
    if (expf == 0x7fffu) {
        if (frac == 0 || frac == 0x8000000000000000ULL) {
            if (sign && is_unsigned) nan_pattern(lo, hi);
            else { *lo = 0; *hi = 0; }
        } else {
            nan_pattern(lo, hi);
        }
        return;
    }
    if (expf == 0) exp = -16382;
    else exp = (int)expf - 16383;
    mag_to_i128(sign, exp, frac, 63, 0, lo, hi);
}

void __fakecc_fixdfti(double a, unsigned long long *lo, unsigned long long *hi) {
    fix_double(a, 0, lo, hi);
}
void __fakecc_fixunsdfti(double a, unsigned long long *lo, unsigned long long *hi) {
    fix_double(a, 1, lo, hi);
}
void __fakecc_fixsfti(float a, unsigned long long *lo, unsigned long long *hi) {
    fix_float(a, 0, lo, hi);
}
void __fakecc_fixunssfti(float a, unsigned long long *lo, unsigned long long *hi) {
    fix_float(a, 1, lo, hi);
}
void __fakecc_fixxfti(long double a, unsigned long long *lo, unsigned long long *hi) {
    fix_ld(a, 0, lo, hi);
}
void __fakecc_fixunsxfti(long double a, unsigned long long *lo, unsigned long long *hi) {
    fix_ld(a, 1, lo, hi);
}
