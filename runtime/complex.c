/* Robust complex division for float/double, on every target.
 *
 * The naive quotient ((ac+bd) + (bc-ad)i)/(c^2+d^2) overflows to inf and
 * underflows to 0 near the exponent limits.  This follows the numerically
 * protected Smith reduction that libgcc's __divdc3 has used since the 2021
 * "practical improvement to complex divide" change: pre-scale all four
 * operands when the pivot denominator component is near the max or min
 * representable, and when the Smith ratio is subnormal (so ratio*operand
 * would underflow by itself), evaluate the cross terms through the
 * pivot reciprocal instead.  gcc.c-torture cdivchkd/cdivchkf pin the
 * boundary behaviour this protects.
 *
 * Used on targets without a hardware float wider than double (arm64
 * Darwin's long double is itself a double).  Results return through two
 * output pointers so neither the SysV nor the arm64 backend needs a
 * complex struct-return special case.  Freestanding throughout (no libm). */
package runtime;

typedef unsigned long long u64c;

static double cf_fabs(double x) {
    union { double d; u64c u; } v;
    v.d = x;
    v.u &= 0x7fffffffffffffffULL;
    return v.d;
}
static int cf_isinf(double x) {
    union { double d; u64c u; } v;
    v.d = x;
    return (v.u & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL;
}
static int cf_isnan(double x) {
    union { double d; u64c u; } v;
    v.d = x;
    u64c a = v.u & 0x7fffffffffffffffULL;
    return a > 0x7ff0000000000000ULL;
}
static int cf_finite(double x) { return !cf_isinf(x) && !cf_isnan(x); }
static double cf_inf(int neg) {
    union { double d; u64c u; } v;
    v.u = neg ? 0xfff0000000000000ULL : 0x7ff0000000000000ULL;
    return v.d;
}
static double cf_copysign(double x, double y) {
    union { double d; u64c u; } a, b;
    a.d = x; b.d = y;
    a.u = (a.u & 0x7fffffffffffffffULL) | (b.u & 0x8000000000000000ULL);
    return a.d;
}

void __fakecc_divdc3(double *pr, double *pi,
                     double ar, double ai, double br, double bi) {
    double a = ar, b = ai, c = br, d = bi;
    double x, y, ratio, denom;

    /* libgcc double-precision thresholds. */
    const double RBIG    = 0x1.fffffffffffffp+1022;  /* DBL_MAX/2 */
    const double RMIN    = 0x1.0p-1022;              /* DBL_MIN */
    const double RMIN2   = 0x1.0p-52;                /* DBL_EPSILON */
    const double RMINSCAL = 0x1.0p+52;               /* 1/DBL_EPSILON */
    const double RMAX2   = 0x1.fffffffffffffp+970;   /* RBIG*RMIN2 */

    if (cf_fabs(c) < cf_fabs(d)) {
        if (cf_fabs(d) >= RBIG) {
            a = a / 2.0; b = b / 2.0; c = c / 2.0; d = d / 2.0;
        }
        if (cf_fabs(d) < RMIN2) {
            a = a * RMINSCAL; b = b * RMINSCAL;
            c = c * RMINSCAL; d = d * RMINSCAL;
        } else if ((cf_fabs(a) < RMIN && cf_fabs(b) < RMAX2
                    && cf_fabs(d) < RMAX2)
                   || (cf_fabs(b) < RMIN && cf_fabs(a) < RMAX2
                       && cf_fabs(d) < RMAX2)) {
            a = a * RMINSCAL; b = b * RMINSCAL;
            c = c * RMINSCAL; d = d * RMINSCAL;
        }
        ratio = c / d;
        denom = (c * ratio) + d;
        if (cf_fabs(ratio) > RMIN) {
            x = ((a * ratio) + b) / denom;
            y = ((b * ratio) - a) / denom;
        } else {
            x = ((c * (a / d)) + b) / denom;
            y = ((c * (b / d)) - a) / denom;
        }
    } else {
        if (cf_fabs(c) >= RBIG) {
            a = a / 2.0; b = b / 2.0; c = c / 2.0; d = d / 2.0;
        }
        if (cf_fabs(c) < RMIN2) {
            a = a * RMINSCAL; b = b * RMINSCAL;
            c = c * RMINSCAL; d = d * RMINSCAL;
        } else if ((cf_fabs(a) < RMIN && cf_fabs(b) < RMAX2
                    && cf_fabs(c) < RMAX2)
                   || (cf_fabs(b) < RMIN && cf_fabs(a) < RMAX2
                       && cf_fabs(c) < RMAX2)) {
            a = a * RMINSCAL; b = b * RMINSCAL;
            c = c * RMINSCAL; d = d * RMINSCAL;
        }
        ratio = d / c;
        denom = (d * ratio) + c;
        if (cf_fabs(ratio) > RMIN) {
            x = ((b * ratio) + a) / denom;
            y = (b - (a * ratio)) / denom;
        } else {
            x = (a + (d * (b / c))) / denom;
            y = (b - (d * (a / c))) / denom;
        }
    }

    /* Recover infinities and zeros that computed as NaN+iNaN: nonzero/0,
     * infinite/finite, finite/infinite. */
    if (cf_isnan(x) && cf_isnan(y)) {
        if (c == 0.0 && d == 0.0 && (!cf_isnan(a) || !cf_isnan(b))) {
            x = cf_copysign(cf_inf(0), c) * a;
            y = cf_copysign(cf_inf(0), c) * b;
        } else if ((cf_isinf(a) || cf_isinf(b))
                   && cf_finite(c) && cf_finite(d)) {
            a = cf_copysign(cf_isinf(a) ? 1.0 : 0.0, a);
            b = cf_copysign(cf_isinf(b) ? 1.0 : 0.0, b);
            x = cf_inf(0) * (a * c + b * d);
            y = cf_inf(0) * (b * c - a * d);
        } else if ((cf_isinf(c) || cf_isinf(d))
                   && cf_finite(a) && cf_finite(b)) {
            c = cf_copysign(cf_isinf(c) ? 1.0 : 0.0, c);
            d = cf_copysign(cf_isinf(d) ? 1.0 : 0.0, d);
            x = 0.0 * (a * c + b * d);
            y = 0.0 * (b * c - a * d);
        }
    }

    *pr = x;
    *pi = y;
}

/* float divide: double-precision scratch keeps every float exponent
 * comfortably in range; narrow at the end. */
void __fakecc_divsc3(float *pr, float *pi,
                     float ar, float ai, float br, float bi) {
    double rr, ri;
    __fakecc_divdc3(&rr, &ri, (double)ar, (double)ai, (double)br, (double)bi);
    *pr = (float)rr;
    *pi = (float)ri;
}
