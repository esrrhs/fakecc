/* exit, abort, atoi, strto*, qsort, chmod — FakeCC dialect. */
package runtime;

void exit(int code) {
    __rt_stdio_init();
    fflush(0);
    __syscall(231, (long)code);
}

void abort(void) {
    __syscall(231, 127);
}

int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }

double fabs(double x) {
    union { double d; unsigned long long u; } ux;
    ux.d = x;
    ux.u = ux.u & 0x7fffffffffffffffULL;
    return ux.d;
}
float fabsf(float x) {
    union { float f; unsigned int u; } ux;
    ux.f = x;
    ux.u = ux.u & 0x7fffffffU;
    return ux.f;
}
long double fabsl(long double x) {
    /* The sign bit sits at the top of the *last* word, which is bit 15 of
     * the second word in x86's 80-bit layout but bit 63 of the only word
     * arm64 gives long double.  Comparing against zero flips the sign the
     * same way on both without naming a bit position. */
    if (x < 0.0L) return -x;
    if (x > 0.0L) return x;
    return x;
}

double copysign(double x, double y) {
    union { double d; unsigned long long u; } ux, uy;
    ux.d = x;
    uy.d = y;
    ux.u = (ux.u & 0x7fffffffffffffffULL) | (uy.u & 0x8000000000000000ULL);
    return ux.d;
}

float copysignf(float x, float y) {
    union { float f; unsigned int u; } ux, uy;
    ux.f = x;
    uy.f = y;
    ux.u = (ux.u & 0x7fffffffU) | (uy.u & 0x80000000U);
    return ux.f;
}

long double copysignl(long double x, long double y) {
    /* See fabsl: naming the sign bit would mean naming x86's 80-bit layout,
     * which arm64 does not share.  Building the magnitude by hand instead
     * works on both. */
    long double mag = (x < 0.0L) ? -x : x;
    if (y < 0.0L) return -mag;
    if (y > 0.0L) return mag;
    return x;                      /* y is -0.0 or +0.0: keep x's sign */
}

double floor(double x) {
    if (x != x) return x;
    if (x == 0.0) return x; /* preserve ±0 */
    if (x >= 9223372036854775807.0 || x <= -9223372036854775807.0) return x;
    if (x > 0.0) return (double)(long long)x;
    long long i = (long long)x;
    if ((double)i == x) return (double)i;
    return (double)(i - 1);
}
float floorf(float x) { return (float)floor((double)x); }

double ceil(double x) {
    if (x != x) return x;
    if (x == 0.0) return x; /* preserve ±0 */
    double f = floor(x);
    if (f == x) return f;
    double r = f + 1.0;
    /* Annex F: ceil of a value in (−1, 0) is −0, not +0 from (−1)+1. */
    if (x < 0.0 && r == 0.0) {
        union { double d; unsigned long long u; } nz;
        nz.u = 0x8000000000000000ULL;
        return nz.d;
    }
    return r;
}

double sqrt(double x) {
    if (x != x) return x;
    if (x < 0.0) {
        union { double d; unsigned long long u; } nanv;
        nanv.u = 0x7ff8000000000000ULL;
        return nanv.d;
    }
    /* +Inf stays +Inf; ±0 keeps its sign (Annex F). */
    if (x > 1.7976931348623157e308) return x;
    if (x == 0.0) return x;
    double g = x;
    int n = 0;
    while (n < 40) {
        g = 0.5 * (g + x / g);
        n = n + 1;
    }
    return g;
}

double sin(double x) {
    if (x == 0.0 || x != x) return x;
    double pi2 = 6.28318530717958647692;
    double pi = 3.14159265358979323846;
    /* ±Inf: Annex F returns NaN.  Also avoid (long long)(Inf / 2π) UB and
     * a non-terminating range-reduction loop for huge finite args. */
    if (x > 1.7976931348623157e308 || x < -1.7976931348623157e308) {
        union { double d; unsigned long long u; } nanv;
        nanv.u = 0x7ff8000000000000ULL;
        return nanv.d;
    }
    double n = floor(x / pi2);
    x = x - n * pi2;
    if (x != x) {
        union { double d; unsigned long long u; } nanv;
        nanv.u = 0x7ff8000000000000ULL;
        return nanv.d;
    }
    int guard = 0;
    while (x > pi && guard < 8) { x = x - pi2; guard = guard + 1; }
    while (x < -pi && guard < 8) { x = x + pi2; guard = guard + 1; }
    if (x == 0.0) return x;
    double term = x;
    double sum = x;
    double x2 = x * x;
    for (int i = 1; i <= 12; i++) {
        term = -term * x2 / (double)((2 * i) * (2 * i + 1));
        sum = sum + term;
    }
    return sum;
}
float sinf(float x) {
    if (x == 0.0f || x != x) return x;
    return (float)sin((double)x);
}

double cos(double x) {
    if (x != x) return x;
    double pi_half = 1.57079632679489661923;
    return sin(pi_half - x);
}
float cosf(float x) { return (float)cos((double)x); }

double tan(double x) {
    if (x == 0.0 || x != x) return x;
    return sin(x) / cos(x);
}
float tanf(float x) {
    if (x == 0.0f || x != x) return x;
    return (float)tan((double)x);
}

double atan(double x) {
    if (x == 0.0 || x != x) return x;
    int neg = 0;
    if (x < 0.0) { neg = 1; x = -x; }
    int inv = 0;
    if (x > 1.0) { inv = 1; x = 1.0 / x; }
    double res = 0.0;
    if (x > 0.4142135623730950) {
        double y = (x - 1.0) / (1.0 + x);
        double term = y;
        double y2 = y * y;
        double s = y;
        for (int i = 1; i <= 15; i++) {
            term = -term * y2;
            s = s + term / (double)(2 * i + 1);
        }
        res = 0.78539816339744830962 + s;
    } else {
        double term = x;
        double x2 = x * x;
        double s = x;
        for (int i = 1; i <= 15; i++) {
            term = -term * x2;
            s = s + term / (double)(2 * i + 1);
        }
        res = s;
    }
    if (inv) res = 1.57079632679489661923 - res;
    if (neg) res = -res;
    return res;
}
float atanf(float x) {
    if (x == 0.0f || x != x) return x;
    return (float)atan((double)x);
}

/* exp / log / pow / atan2 / asin / acos.
 *
 * pow is what the x86 build got from the host libm and a freestanding arm64
 * image has to carry itself: gcc_torture declares it extern and really calls
 * it, and the Mach-O linker turns an unresolved symbol into a hard error
 * instead of deferring to dyld.  exp and log are the pair it is built from.
 *
 * Both need range reduction.  A Taylor series for e^x only converges near
 * the origin, so the exponent is split off and re-applied as an exact power
 * of two; log reduces its argument to [1,2) before using the atanh series,
 * whose argument then stays below 1/3. */

/* 2^k, exact.  Scaling by repeated multiplication would overflow for the k a
 * large argument produces, so write the biased exponent directly; a k below
 * the normal range is built from the smallest normal times the remainder. */
static double rt_pow2(int k) {
    union { double d; unsigned long long u; } v;
    if (k > 1023) {
        v.u = 0x7ff0000000000000ULL;
        return v.d;
    }
    if (k < -1074) return 0.0;
    if (k < -1022) {
        v.u = 0x0010000000000000ULL;          /* 2^-1022 */
        return v.d * rt_pow2(k + 1022);
    }
    v.u = (unsigned long long)(k + 1023) << 52;
    return v.d;
}

double exp(double x) {
    if (x != x) return x;
    if (x == 0.0) return 1.0;
    union { double d; unsigned long long u; } inf;
    inf.u = 0x7ff0000000000000ULL;
    if (x > 709.7827128933840) return inf.d;
    if (x < -745.1332191019411) return 0.0;
    double ln2 = 0.69314718055994530942;
    int k = (int)floor(x / ln2 + 0.5);
    double r = x - (double)k * ln2;           /* |r| <= ln2/2 */
    double term = 1.0;
    double sum = 1.0;
    for (int i = 1; i <= 30; i++) {
        term = term * r / (double)i;
        sum = sum + term;
    }
    return sum * rt_pow2(k);
}

double log(double x) {
    if (x != x) return x;
    if (x > 1.7976931348623157e308) return x;   /* +Inf stays +Inf */
    if (x == 0.0) {
        union { double d; unsigned long long u; } v;
        v.u = 0xfff0000000000000ULL;            /* -Inf */
        return v.d;
    }
    if (x < 0.0) {
        union { double d; unsigned long long u; } v;
        v.u = 0x7ff8000000000000ULL;            /* NaN */
        return v.d;
    }
    union { double d; unsigned long long u; } v;
    v.d = x;
    int be = (int)((v.u >> 52) & 0x7ffULL);
    int e2 = 0;
    if (be == 0) {                              /* subnormal */
        v.d = x * 4503599627370496.0;           /* 2^52, brings it into range */
        e2 = -52;
        be = (int)((v.u >> 52) & 0x7ffULL);
    }
    e2 = e2 + be - 1023;
    v.u = (v.u & 0x000fffffffffffffULL) | (1023ULL << 52);
    double m = v.d;                             /* [1,2) */
    /* log(m) = 2*atanh((m-1)/(m+1)). */
    double z = (m - 1.0) / (m + 1.0);
    double z2 = z * z;
    double term = z;
    double sum = z;
    for (int i = 1; i <= 40; i++) {
        term = term * z2;
        sum = sum + term / (double)(2 * i + 1);
    }
    return (double)e2 * 0.69314718055994530942 + 2.0 * sum;
}

double log10(double x) {
    return log(x) / 2.30258509299404568402;
}

double pow(double x, double y) {
    if (y == 0.0) return 1.0;
    if (x != x) return x;
    if (y != y) return y;
    if (x == 1.0) return 1.0;
    /* An integral exponent is exact under repeated squaring, and it is the
     * only case where a negative base has a real result at all. */
    if (y > -1024.0 && y < 1024.0 && y == floor(y)) {
        long long n = (long long)y;
        int neg = 0;
        if (n < 0) { neg = 1; n = -n; }
        double base = x;
        double r = 1.0;
        while (n > 0) {
            if (n & 1) r = r * base;
            base = base * base;
            n = n >> 1;
        }
        if (neg) r = 1.0 / r;
        return r;
    }
    if (x == 0.0) {
        union { double d; unsigned long long u; } v;
        v.u = y > 0.0 ? 0x0000000000000000ULL : 0x7ff0000000000000ULL;
        return v.d;
    }
    if (x < 0.0) {
        union { double d; unsigned long long u; } v;
        v.u = 0x7ff8000000000000ULL;
        return v.d;
    }
    if (x > 1.7976931348623157e308) return y > 0.0 ? x : 0.0;
    return exp(y * log(x));
}

double atan2(double y, double x) {
    if (x != x || y != y) {
        union { double d; unsigned long long u; } v;
        v.u = 0x7ff8000000000000ULL;
        return v.d;
    }
    double pi = 3.14159265358979323846;
    double pi_half = 1.57079632679489661923;
    if (x > 0.0) return atan(y / x);
    if (x < 0.0) {
        if (y >= 0.0) return atan(y / x) + pi;
        return atan(y / x) - pi;
    }
    if (y > 0.0) return pi_half;
    if (y < 0.0) return -pi_half;
    return 0.0;
}

double asin(double x) {
    if (x != x) return x;
    if (x > 1.0 || x < -1.0) {
        union { double d; unsigned long long u; } v;
        v.u = 0x7ff8000000000000ULL;
        return v.d;
    }
    double pi_half = 1.57079632679489661923;
    if (x == 1.0) return pi_half;
    if (x == -1.0) return -pi_half;
    /* atan2(x, sqrt(1-x*x)) rather than atan(x / sqrt(...)): the quotient
     * has no quadrant, and (1-x)*(1+x) keeps the cancellation away from
     * x = 1, where 1-x*x has already lost half its digits. */
    return atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}

double acos(double x) {
    if (x != x) return x;
    return 1.57079632679489661923 - asin(x);
}

/* setjmp / longjmp.
 *
 * Saving the frame pointer and the return address is not expressible in the
 * language, so the backend lowers __builtin_setjmp / __builtin_longjmp
 * itself (IR_FRAME_ADDR / IR_RETURN_ADDR / IR_LONGJMP, all of which cg64
 * implements) and the libc names are one-line wrappers.  jmp_buf belongs to
 * the caller: an array of long large enough for the saved frame, fp, lr and
 * the callee-saved registers. */
int setjmp(long *env) {
    return __builtin_setjmp((void *)env);
}
void longjmp(long *env, int val) {
    __builtin_longjmp((void *)env, val);
}


/* Shared body of the strto* family: parses [ws][sign][base prefix][digits] and
 * returns the magnitude, with the sign reported through *neg.  On overflow
 * *ovf is set and the returned magnitude is ULLONG_MAX (glibc saturates). */
static unsigned long long strtou_body(const char *s, char **end, int base,
                                      int *neg, int *ovf) {
    const char *nptr = s;
    *ovf = 0;
    while (isspace((unsigned char)*s)) s = s + 1;
    *neg = 0;
    if (*s == '+') s = s + 1;
    else if (*s == '-') {
        *neg = 1;
        s = s + 1;
    }
    if (base != 0 && (base < 2 || base > 36)) {
        if (end) *end = (char *)nptr;
        return 0;
    }
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X') && isxdigit((unsigned char)s[2])) {
            base = 16;
            s = s + 2;
        } else if (s[0] == '0') base = 8;
        else base = 10;
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')
               && isxdigit((unsigned char)s[2])) {
        s = s + 2;
    }
    unsigned long long v = 0;
    unsigned long long ubase = (unsigned long long)base;
    unsigned long long umax = 18446744073709551615ULL;
    int any = 0;
    while (1) {
        int d;
        char c = *s;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        else break;
        if (d >= base) break;
        any = 1;
        if (!*ovf) {
            if (v > umax / ubase || (v == umax / ubase && (unsigned long long)d > umax % ubase))
                *ovf = 1;
            else
                v = v * ubase + (unsigned long long)d;
        }
        s = s + 1;
    }
    if (!any) {
        if (end) *end = (char *)nptr;
        return 0;
    }
    if (end) *end = (char *)s;
    if (*ovf) return umax;
    return v;
}

long strtol(const char *s, char **end, int base) {
    int neg;
    int ovf;
    unsigned long long v = strtou_body(s, end, base, &neg, &ovf);
    unsigned long long lim_pos = 9223372036854775807ULL;
    unsigned long long lim_neg = 9223372036854775808ULL;
    if (ovf || (!neg && v > lim_pos) || (neg && v > lim_neg)) {
        errno = 34;
        if (neg) return -9223372036854775807L - 1L;
        return 9223372036854775807L;
    }
    if (neg) return -(long)v;
    return (long)v;
}

long long strtoll(const char *s, char **end, int base) {
    return (long long)strtol(s, end, base);
}

unsigned long long strtoull(const char *s, char **end, int base) {
    int neg;
    int ovf;
    unsigned long long v = strtou_body(s, end, base, &neg, &ovf);
    if (ovf) {
        errno = 34;
        return 18446744073709551615ULL;
    }
    if (neg) return 0ULL - v;
    return v;
}

unsigned long strtoul(const char *s, char **end, int base) {
    return (unsigned long)strtoull(s, end, base);
}

int atoi(const char *s) {
    return (int)strtol(s, 0, 10);
}

long atol(const char *s) {
    return strtol(s, 0, 10);
}

/* 10^k as an exact long double, for k <= LD_POW10_EXACT.  10^27 = 2^27 * 5^27
 * and 5^27 < 2^64, so every power up to that fits the x87 64-bit mantissa with
 * no rounding; the loop below therefore only multiplies exact values. */
static long double pow10_exact(int k) {
    long double p = 1.0L;
    while (k > 0) {
        p = p * 10.0L;
        k = k - 1;
    }
    return p;
}

/* Parse a decimal floating literal by accumulating the digits as an exact
 * integer mantissa and applying the decimal exponent in as few scalings as
 * possible.  Scaling digit by digit (v += 0.1 * d, place *= 0.1) instead
 * compounds the representation error of 0.1 across every fraction digit, so
 * "7.125" — a value with an exact binary form — came back as 7.12499...  The
 * compiler parses every float literal in its input through here, so that error
 * would land in the constants of every program the self-hosted compiler builds.
 *
 * Only integer-valued literals appear below, which parse exactly under both
 * this implementation and the host's, keeping the bootstrap a fixed point. */
static int hex_char_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static long double strtofp_body(const char *s, char **end) {
    const char *start = s;
    while (isspace((unsigned char)*s)) s = s + 1;
    int neg = 0;
    if (*s == '+') s = s + 1;
    else if (*s == '-') {
        neg = 1;
        s = s + 1;
    }

    /* INF / INFINITY / NAN (C99).  Case-insensitive.  Never read past NUL. */
    {
        char c0 = s[0];
        char c1 = c0 ? s[1] : 0;
        char c2 = c1 ? s[2] : 0;
        if (c0 >= 'A' && c0 <= 'Z') c0 = (char)(c0 - 'A' + 'a');
        if (c1 >= 'A' && c1 <= 'Z') c1 = (char)(c1 - 'A' + 'a');
        if (c2 >= 'A' && c2 <= 'Z') c2 = (char)(c2 - 'A' + 'a');
        if (c0 == 'i' && c1 == 'n' && c2 == 'f') {
            s = s + 3;
            /* optional "inity" */
            char w0 = s[0];
            char w1 = w0 ? s[1] : 0;
            char w2 = w1 ? s[2] : 0;
            char w3 = w2 ? s[3] : 0;
            char w4 = w3 ? s[4] : 0;
            if (w0 >= 'A' && w0 <= 'Z') w0 = (char)(w0 - 'A' + 'a');
            if (w1 >= 'A' && w1 <= 'Z') w1 = (char)(w1 - 'A' + 'a');
            if (w2 >= 'A' && w2 <= 'Z') w2 = (char)(w2 - 'A' + 'a');
            if (w3 >= 'A' && w3 <= 'Z') w3 = (char)(w3 - 'A' + 'a');
            if (w4 >= 'A' && w4 <= 'Z') w4 = (char)(w4 - 'A' + 'a');
            if (w0 == 'i' && w1 == 'n' && w2 == 'i' && w3 == 't' && w4 == 'y')
                s = s + 5;
            if (end) *end = (char *)s;
            /* long double is 80-bit on x86-64 but the same 64 bits as double
             * on arm64, so a fixed bit pattern cannot serve both.  Dividing
             * a non-zero by zero raises the trap the hardware defines, which
             * every target agrees on. */
            {
                long double one = 1.0L;
                long double inf = one / 0.0L;
                return neg ? -inf : inf;
            }
        }
        if (c0 == 'n' && c1 == 'a' && c2 == 'n') {
            s = s + 3;
            if (*s == '(') {
                const char *p = s + 1;
                while (*p && *p != ')') p = p + 1;
                if (*p == ')') s = p + 1;
                /* else leave s at '(' so endptr is after "nan", matching C99 */
            }
            if (end) *end = (char *)s;
            {
                long double zero = 0.0L;
                long double nan = zero / zero;
                return neg ? -nan : nan;
            }
        }
    }

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        const char *h = s + 2;
        int hex_ok = 0;
        if (hex_char_val(*h) >= 0) hex_ok = 1;
        else if (*h == '.' && hex_char_val(h[1]) >= 0) hex_ok = 1;
        if (!hex_ok) {
            /* "0x" without a hex digit is a decimal 0, remainder at 'x'. */
        } else {
        s = s + 2;
        long double mant = 0.0L;
        int any = 0;
        int shift_bits = 0;
        for (;;) {
            int v = hex_char_val(*s);
            if (v < 0) break;
            any = 1;
            mant = mant * 16.0L + (long double)v;
            s = s + 1;
        }
        if (*s == '.') {
            s = s + 1;
            for (;;) {
                int v = hex_char_val(*s);
                if (v < 0) break;
                any = 1;
                mant = mant * 16.0L + (long double)v;
                shift_bits = shift_bits - 4;
                s = s + 1;
            }
        }
        if (!any) {
            if (end) *end = (char *)start;
            return 0.0L;
        }
        int bin_exp = 0;
        if (*s == 'p' || *s == 'P') {
            const char *ppos = s;
            s = s + 1;
            int pneg = 0;
            if (*s == '+') s = s + 1;
            else if (*s == '-') {
                pneg = 1;
                s = s + 1;
            }
            if (isdigit((unsigned char)*s)) {
                while (isdigit((unsigned char)*s)) {
                    if (bin_exp < 100000) bin_exp = bin_exp * 10 + (*s - '0');
                    s = s + 1;
                }
                if (pneg) bin_exp = -bin_exp;
            } else {
                s = ppos;
            }
        }
        if (end) *end = (char *)s;
        int total_exp = bin_exp + shift_bits;
        while (total_exp >= 32) {
            mant = mant * 4294967296.0L;
            total_exp = total_exp - 32;
        }
        while (total_exp > 0) {
            mant = mant * 2.0L;
            total_exp = total_exp - 1;
        }
        while (total_exp <= -32) {
            mant = mant / 4294967296.0L;
            total_exp = total_exp + 32;
        }
        while (total_exp < 0) {
            mant = mant / 2.0L;
            total_exp = total_exp + 1;
        }
        if (neg) mant = -mant;
        return mant;
        }
    }

    long double mant = 0.0L;
    int any = 0;
    int ndig = 0;   /* digits folded into mant; 80-bit ld is exact to 2^64 */
    int dexp = 0;   /* power of ten still to apply to mant */
    while (isdigit((unsigned char)*s)) {
        any = 1;
        /* Cap at 21 digits (not 19): 2^64 is 20 digits and must stay exact.
         * Extra digits beyond that become a power of ten, like rounding zeros. */
        if (ndig < 21) {
            mant = mant * 10.0L + (long double)(*s - '0');
            ndig = ndig + 1;
        } else {
            dexp = dexp + 1;
        }
        s = s + 1;
    }
    if (*s == '.') {
        s = s + 1;
        while (isdigit((unsigned char)*s)) {
            any = 1;
            if (ndig < 21) {
                mant = mant * 10.0L + (long double)(*s - '0');
                ndig = ndig + 1;
                dexp = dexp - 1;
            }
            s = s + 1;
        }
    }
    if (!any) {
        if (end) *end = (char *)start;
        return 0.0L;
    }

    if (*s == 'e' || *s == 'E') {
        const char *epos = s;
        s = s + 1;
        int eneg = 0;
        if (*s == '+') s = s + 1;
        else if (*s == '-') {
            eneg = 1;
            s = s + 1;
        }
        if (isdigit((unsigned char)*s)) {
            int exp = 0;
            while (isdigit((unsigned char)*s)) {
                if (exp < 100000) exp = exp * 10 + (*s - '0');
                s = s + 1;
            }
            if (eneg) dexp = dexp - exp;
            else dexp = dexp + exp;
        } else {
            s = epos;   /* no digits after 'e': the exponent is not part of it */
        }
    }
    if (end) *end = (char *)s;

    /* Scale by powers of ten without leaping past the finite range into
     * +inf.  Near LDBL_MAX (~1.19e4932), mant*10^27 can round up to
     * infinity even when the mathematical value is still finite — then
     * self-hosted fakecc embeds LDBL_MAX literals as inf (ieee/pr36332).
     * Clamp that rounding overflow to the finite max; only a clearly
     * out-of-range mantissa becomes ±inf. */
    {
        long double ld_max = 0xf.fffffffffffffffp+16380L;
        long double pos_inf = 1.0L / 0.0L;
        while (dexp > 0) {
            int step = dexp;
            if (step > 27) step = 27;
            long double factor = pow10_exact(step);
            long double next = mant * factor;
            if (next > ld_max) {
                /* ld_max/factor rounds down, and prior *10^27 steps add a
                 * ULP or two, so an in-range LDBL_MAX input can look
                 * slightly past the limit.  Pad by ~2^-60 relatively. */
                long double lim = ld_max / factor;
                long double lim_pad = lim + lim * 0x1p-60L;
                if (mant > lim_pad)
                    mant = pos_inf;
                else
                    mant = ld_max;
                dexp = 0;
                break;
            }
            mant = next;
            dexp = dexp - step;
        }
        while (dexp < 0) {
            int step = -dexp;
            if (step > 27) step = 27;
            mant = mant / pow10_exact(step);
            dexp = dexp + step;
        }
    }

    if (neg) mant = -mant;
    return mant;
}

double strtod(const char *s, char **end) {
    return (double)strtofp_body(s, end);
}

float strtof(const char *s, char **end) {
    return (float)strtofp_body(s, end);
}

long double strtold(const char *s, char **end) {
    return strtofp_body(s, end);
}

static void qsort_swap(char *a, char *b, size_t sz) {
    char tmp[64];
    size_t left = sz;
    while (left > 0) {
        size_t n = left;
        if (n > 64) n = 64;
        memcpy(tmp, a, n);
        memcpy(a, b, n);
        memcpy(b, tmp, n);
        a = a + n;
        b = b + n;
        left = left - n;
    }
}

static void qsort_rec(char *base, size_t n, size_t sz,
                      int (*cmp)(const void *, const void *)) {
    /* Lomuto partition: pivot = last element.  Every element left of the
     * final pivot slot is < pivot, every element right is >= pivot, so the
     * pivot lands at a strictly interior index and both sides are smaller
     * than n — the old Hoare scheme could return mid == n on a sorted pair
     * (pivot == max), recursing on (base, n) forever.  Tail-call the larger
     * partition so stack depth stays O(log n). */
    while (n > 1) {
        char *pivot = base + (n - 1) * sz;
        size_t i;
        size_t store = 0;
        for (i = 0; i < n - 1; i = i + 1) {
            if (cmp(base + i * sz, pivot) < 0) {
                if (i != store) qsort_swap(base + i * sz, base + store * sz, sz);
                store = store + 1;
            }
        }
        if (store != n - 1) qsort_swap(base + store * sz, pivot, sz);
        if (store < n - store - 1) {
            qsort_rec(base, store, sz, cmp);
            base = base + (store + 1) * sz;
            n = n - store - 1;
        } else {
            qsort_rec(base + (store + 1) * sz, n - store - 1, sz, cmp);
            n = store;
        }
    }
}

void qsort(void *base, size_t n, size_t sz, int (*cmp)(const void *, const void *)) {
    if (base == 0 || n < 2 || sz == 0 || cmp == 0) return;
    qsort_rec((char *)base, n, sz, cmp);
}

/* chmod() and getenv() are platform-specific:
 *   - Linux builds use the raw syscall numbers / /proc/self/environ
 *     (see stdlib_linux.c);
 *   - Darwin builds leave the names undefined so the arm64 backend emits
 *     its own chmod syscall and the envp-scanning getenv builtin.
 * Keeping the Linux bodies here would be selected on every target (this
 * file is un-suffixed) and shadow the correct Darwin implementations. */

/* The va_copy helper is target-specific and lives in stdlib_darwin.c /
 * stdlib_linux.c: a va_list is 24 bytes on x86-64 but a single pointer on
 * arm64, so the copy width cannot be shared. */
