/* exit, abort, atoi, strto*, qsort, chmod — FakeCC dialect. */
package runtime;

void exit(int code) {
    __rt_stdio_init();
    fflush(stdout);
    fflush(stderr);
    __syscall(231, (long)code);
}

void abort(void) {
    __syscall(231, 127);
}

double fabs(double x) { return x < 0.0 ? -x : x; }
float fabsf(float x) { return x < 0.0f ? -x : x; }

double floor(double x) {
    if (x >= 0.0) return (double)(long long)x;
    long long i = (long long)x;
    if ((double)i == x) return (double)i;
    return (double)(i - 1);
}
float floorf(float x) { return (float)floor((double)x); }

double ceil(double x) {
    double f = floor(x);
    if (f == x) return f;
    return f + 1.0;
}

double sqrt(double x) {
    if (x <= 0.0) return 0.0;
    double g = x;
    int n = 0;
    while (n < 40) {
        g = 0.5 * (g + x / g);
        n = n + 1;
    }
    return g;
}


/* Shared body of the strto* family: parses [ws][sign][base prefix][digits] and
 * returns the magnitude, with the sign reported through *neg.  Wraparound on
 * overflow rather than clamping to LONG_MAX/ULLONG_MAX. */
static unsigned long long strtou_body(const char *s, char **end, int base,
                                      int *neg) {
    while (isspace((unsigned char)*s)) s = s + 1;
    *neg = 0;
    if (*s == '+') s = s + 1;
    else if (*s == '-') {
        *neg = 1;
        s = s + 1;
    }
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s = s + 2;
        } else if (s[0] == '0') base = 8;
        else base = 10;
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s = s + 2;
    }
    unsigned long long v = 0;
    while (1) {
        int d;
        char c = *s;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        else break;
        if (d >= base) break;
        v = v * (unsigned long long)base + (unsigned long long)d;
        s = s + 1;
    }
    if (end) *end = (char *)s;
    return v;
}

long strtol(const char *s, char **end, int base) {
    int neg;
    unsigned long long v = strtou_body(s, end, base, &neg);
    if (neg) return -(long)v;
    return (long)v;
}

unsigned long long strtoull(const char *s, char **end, int base) {
    int neg;
    unsigned long long v = strtou_body(s, end, base, &neg);
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
static long double strtofp_body(const char *s, char **end) {
    const char *start = s;
    while (isspace((unsigned char)*s)) s = s + 1;
    int neg = 0;
    if (*s == '+') s = s + 1;
    else if (*s == '-') {
        neg = 1;
        s = s + 1;
    }

    long double mant = 0.0L;
    int any = 0;
    int ndig = 0;   /* digits folded into mant; 19 exceeds the mantissa */
    int dexp = 0;   /* power of ten still to apply to mant */
    while (isdigit((unsigned char)*s)) {
        any = 1;
        if (ndig < 19) {
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
            if (ndig < 19) {
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

    long double chunk = pow10_exact(27);
    while (dexp >= 27) {
        mant = mant * chunk;
        dexp = dexp - 27;
    }
    while (dexp <= -27) {
        mant = mant / chunk;
        dexp = dexp + 27;
    }
    if (dexp > 0) mant = mant * pow10_exact(dexp);
    else if (dexp < 0) mant = mant / pow10_exact(-dexp);

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

int chmod(const char *path, int mode) {
    long r = __syscall(90, (long)path, (long)mode);
    return r < 0 ? -1 : 0;
}

/* Scan /proc/self/environ.  Returned pointer is into a static buffer. */
char *getenv(const char *name) {
    static char block[8192];
    size_t nlen = 0;
    while (name[nlen]) nlen = nlen + 1;
    long fd = __syscall(2, (long)"/proc/self/environ", 0, 0);
    if (fd < 0) return 0;
    long n = __syscall(0, fd, (long)block, 8191);
    __syscall(3, fd);
    if (n <= 0) return 0;
    if (n > 8191) n = 8191;
    block[n] = 0;
    char *p = block;
    while ((unsigned long)(p - block) < (unsigned long)n) {
        if (*p == 0) {
            p = p + 1;
            continue;
        }
        size_t i = 0;
        while (p[i] && p[i] != '=') i = i + 1;
        if (p[i] == '=' && i == nlen) {
            int match = 1;
            size_t j = 0;
            while (j < nlen) {
                if (p[j] != name[j]) {
                    match = 0;
                    break;
                }
                j = j + 1;
            }
            if (match) return p + i + 1;
        }
        while (*p) p = p + 1;
        p = p + 1;
    }
    return 0;
}

void __fakecc_va_copy(void *dst, void *src) {
    char *d = (char *)dst;
    char *s = (char *)src;
    int i = 0;
    while (i < 24) {
        d[i] = s[i];
        i = i + 1;
    }
}
