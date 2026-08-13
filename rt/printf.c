/* printf family — FakeCC dialect. Supports %s %c %d %i %u %x %p %ld %lu %lld %% and width. */
package main;

typedef unsigned long size_t;
typedef struct FILE FILE;

extern FILE *stdout;
extern FILE *stderr;
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern size_t strlen(const char *s);
extern void *__fakecc_va_copy(void *dst, void *src);
extern void *malloc(size_t n);
extern void free(void *p);

static void ensure_stdio(void) {
    extern void __rt_stdio_init(void);
    __rt_stdio_init();
}

static int emit_ch(char **bufp, size_t *left, int *count, FILE *f, char ch) {
    *count = *count + 1;
    if (bufp && *bufp) {
        if (*left > 1) {
            **bufp = ch;
            *bufp = *bufp + 1;
            *left = *left - 1;
        } else if (*left == 1) {
            /* reserve NUL */
        }
        return 0;
    }
    if (f) {
        if (fputc((unsigned char)ch, f) < 0) return -1;
    }
    return 0;
}

static int emit_str(char **bufp, size_t *left, int *count, FILE *f,
                    const char *s, int precision) {
    int i = 0;
    while (s[i] && (precision < 0 || i < precision)) {
        if (emit_ch(bufp, left, count, f, s[i]) < 0) return -1;
        i = i + 1;
    }
    return 0;
}

static int emit_uint(char **bufp, size_t *left, int *count, FILE *f,
                     unsigned long long v, int base, int upper) {
    char tmp[32];
    int n = 0;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            int d = (int)(v % (unsigned long long)base);
            v = v / (unsigned long long)base;
            if (d < 10) tmp[n] = (char)('0' + d);
            else if (upper) tmp[n] = (char)('A' + d - 10);
            else tmp[n] = (char)('a' + d - 10);
            n = n + 1;
        }
    }
    while (n > 0) {
        n = n - 1;
        if (emit_ch(bufp, left, count, f, tmp[n]) < 0) return -1;
    }
    return 0;
}

static int emit_int(char **bufp, size_t *left, int *count, FILE *f,
                    long long v, int base) {
    unsigned long long u;
    if (v < 0) {
        if (emit_ch(bufp, left, count, f, '-') < 0) return -1;
        u = (unsigned long long)(-v);
    } else {
        u = (unsigned long long)v;
    }
    return emit_uint(bufp, left, count, f, u, base, 0);
}

static int pad(char **bufp, size_t *left, int *count, FILE *f, int n, char ch) {
    while (n > 0) {
        if (emit_ch(bufp, left, count, f, ch) < 0) return -1;
        n = n - 1;
    }
    return 0;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    ensure_stdio();
    char *bp = buf;
    size_t left = n;
    int count = 0;
    int i = 0;
    while (fmt[i]) {
        if (fmt[i] != '%') {
            if (emit_ch(&bp, &left, &count, 0, fmt[i]) < 0) return -1;
            i = i + 1;
            continue;
        }
        i = i + 1;
        if (fmt[i] == '%') {
            if (emit_ch(&bp, &left, &count, 0, '%') < 0) return -1;
            i = i + 1;
            continue;
        }
        int width = 0;
        int precision = -1;
        int long_count = 0;
        while (fmt[i] >= '0' && fmt[i] <= '9') {
            width = width * 10 + (fmt[i] - '0');
            i = i + 1;
        }
        if (fmt[i] == '.') {
            i = i + 1;
            precision = 0;
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                precision = precision * 10 + (fmt[i] - '0');
                i = i + 1;
            }
        }
        while (fmt[i] == 'l') {
            long_count = long_count + 1;
            i = i + 1;
        }
        char spec = fmt[i];
        if (spec == 0) break;
        i = i + 1;
        if (spec == 's') {
            char *s = va_arg(ap, char *);
            if (s == 0) s = "(null)";
            int sl = (int)strlen(s);
            if (precision >= 0 && sl > precision) sl = precision;
            if (width > sl) {
                if (pad(&bp, &left, &count, 0, width - sl, ' ') < 0) return -1;
            }
            if (emit_str(&bp, &left, &count, 0, s, precision) < 0) return -1;
        } else if (spec == 'c') {
            int ch = va_arg(ap, int);
            if (width > 1) {
                if (pad(&bp, &left, &count, 0, width - 1, ' ') < 0) return -1;
            }
            if (emit_ch(&bp, &left, &count, 0, (char)ch) < 0) return -1;
        } else if (spec == 'd' || spec == 'i') {
            long long v;
            if (long_count >= 2) v = va_arg(ap, long long);
            else if (long_count == 1) v = (long long)va_arg(ap, long);
            else v = (long long)va_arg(ap, int);
            if (emit_int(&bp, &left, &count, 0, v, 10) < 0) return -1;
        } else if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'p') {
            unsigned long long v;
            int base = 10;
            int upper = 0;
            if (spec == 'x') base = 16;
            if (spec == 'X') { base = 16; upper = 1; }
            if (spec == 'p') {
                base = 16;
                if (emit_str(&bp, &left, &count, 0, "0x", -1) < 0) return -1;
                v = (unsigned long long)(unsigned long)va_arg(ap, void *);
            } else if (long_count >= 2) v = va_arg(ap, unsigned long long);
            else if (long_count == 1) v = (unsigned long long)va_arg(ap, unsigned long);
            else v = (unsigned long long)va_arg(ap, unsigned int);
            if (emit_uint(&bp, &left, &count, 0, v, base, upper) < 0) return -1;
        } else {
            if (emit_ch(&bp, &left, &count, 0, '%') < 0) return -1;
            if (emit_ch(&bp, &left, &count, 0, spec) < 0) return -1;
        }
    }
    if (buf && n > 0) {
        size_t used = (size_t)(bp - buf);
        if (used >= n) used = n - 1;
        buf[used] = 0;
    }
    return count;
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
    /* Size then emit — simple two-pass using a stack buffer for small, heap for large. */
    va_list ap2;
    __fakecc_va_copy((void *)&ap2, (void *)&ap);
    int need = vsnprintf(0, 0, fmt, ap2);
    va_end(ap2);
    if (need < 0) return -1;
    char stack[512];
    char *mem = stack;
    int heap = 0;
    if (need + 1 > 512) {
        mem = (char *)malloc((size_t)need + 1);
        if (mem == 0) return -1;
        heap = 1;
    }
    vsnprintf(mem, (size_t)need + 1, fmt, ap);
    int i = 0;
    while (i < need) {
        if (fputc((unsigned char)mem[i], f) < 0) {
            if (heap) free(mem);
            return -1;
        }
        i = i + 1;
    }
    if (heap) free(mem);
    return need;
}

int vsprintf(char *buf, const char *fmt, va_list ap) {
    return vsnprintf(buf, (size_t)-1 / 2, fmt, ap);
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, (size_t)-1 / 2, fmt, ap);
    va_end(ap);
    return r;
}

int fprintf(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

int printf(const char *fmt, ...) {
    ensure_stdio();
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return r;
}
