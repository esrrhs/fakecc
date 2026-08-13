/* exit, abort, atoi, strto*, qsort, chmod — FakeCC dialect. */
package main;

typedef unsigned long size_t;
typedef struct FILE FILE;

extern int fflush(FILE *f);
extern FILE *stdout;
extern FILE *stderr;
extern void free(void *p);
extern void *malloc(size_t n);
extern void *memcpy(void *dst, const void *src, size_t n);
extern int isspace(int c);
extern int isdigit(int c);

void exit(int code) {
    extern void __rt_stdio_init(void);
    __rt_stdio_init();
    fflush(stdout);
    fflush(stderr);
    __syscall(231, (long)code);
}

void abort(void) {
    __syscall(231, 127);
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

double strtod(const char *s, char **end) {
    while (isspace((unsigned char)*s)) s = s + 1;
    int neg = 0;
    if (*s == '+') s = s + 1;
    else if (*s == '-') {
        neg = 1;
        s = s + 1;
    }
    double v = 0.0;
    int any = 0;
    while (isdigit((unsigned char)*s)) {
        any = 1;
        v = v * 10.0 + (double)(*s - '0');
        s = s + 1;
    }
    if (*s == '.') {
        s = s + 1;
        double place = 0.1;
        while (isdigit((unsigned char)*s)) {
            any = 1;
            v = v + place * (double)(*s - '0');
            place = place * 0.1;
            s = s + 1;
        }
    }
    if (*s == 'e' || *s == 'E') {
        s = s + 1;
        int eneg = 0;
        if (*s == '+') s = s + 1;
        else if (*s == '-') {
            eneg = 1;
            s = s + 1;
        }
        int exp = 0;
        while (isdigit((unsigned char)*s)) {
            exp = exp * 10 + (*s - '0');
            s = s + 1;
        }
        double pow10 = 1.0;
        int i = 0;
        while (i < exp) {
            pow10 = pow10 * 10.0;
            i = i + 1;
        }
        if (eneg) v = v / pow10;
        else v = v * pow10;
    }
    if (end) *end = (char *)s;
    if (neg) v = -v;
    if (!any) return 0.0;
    return v;
}

long double strtold(const char *s, char **end) {
    return (long double)strtod(s, end);
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
    if (n < 2) return;
    char *pivot = base + (n / 2) * sz;
    size_t i = 0;
    size_t j = n - 1;
    while (1) {
        while (cmp(base + i * sz, pivot) < 0) i = i + 1;
        while (cmp(base + j * sz, pivot) > 0) j = j - 1;
        if (i >= j) break;
        qsort_swap(base + i * sz, base + j * sz, sz);
        if (pivot == base + i * sz) pivot = base + j * sz;
        else if (pivot == base + j * sz) pivot = base + i * sz;
        i = i + 1;
        if (j == 0) break;
        j = j - 1;
    }
    size_t mid = j + 1;
    qsort_rec(base, mid, sz, cmp);
    qsort_rec(base + mid * sz, n - mid, sz, cmp);
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
