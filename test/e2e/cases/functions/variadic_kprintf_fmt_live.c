// expect: 42
/* Must M3 (fakeos docs/fakecc-support-requests.md): -O1 mem2reg must keep
 * the format pointer live across va_arg in a printf-style walk with helper
 * calls — the pattern that forced fakeos onto -O0. */
package main;

static char obuf[256];
static int olen;

static void print_char(char c) {
    if (olen < 255) obuf[olen++] = c;
}
static void print_string(const char *s) {
    if (!s) s = "(null)";
    while (*s) print_char(*s++);
}
static void print_udec(unsigned long val) {
    char buf[32];
    int i = 0;
    if (val == 0) { print_char('0'); return; }
    while (val > 0) {
        buf[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    while (i > 0) print_char(buf[--i]);
}
static void print_dec(long val) {
    if (val < 0) {
        print_char('-');
        print_udec((unsigned long)(-val));
    } else {
        print_udec((unsigned long)val);
    }
}
static void print_hex(unsigned long val) {
    char buf[32];
    int i = 0;
    const char *digits = "0123456789abcdef";
    if (val == 0) { print_char('0'); return; }
    while (val > 0) {
        buf[i++] = digits[val & 0x0F];
        val >>= 4;
    }
    while (i > 0) print_char(buf[--i]);
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;
            if (*fmt == 'c') {
                char c = (char)va_arg(ap, int);
                print_char(c);
            } else if (*fmt == 's') {
                const char *s = va_arg(ap, const char *);
                print_string(s);
            } else if (*fmt == 'd' || *fmt == 'i') {
                long val = (long)va_arg(ap, long long);
                print_dec(val);
            } else if (*fmt == 'u') {
                unsigned long val = (unsigned long)va_arg(ap, unsigned long long);
                print_udec(val);
            } else if (*fmt == 'x') {
                unsigned long val = (unsigned long)va_arg(ap, unsigned long long);
                print_hex(val);
            } else if (*fmt == 'p') {
                unsigned long val = (unsigned long)va_arg(ap, void *);
                print_string("0x");
                print_hex(val);
            } else if (*fmt == '%') {
                print_char('%');
            } else {
                print_char('%');
                print_char(*fmt);
            }
        } else {
            print_char(*fmt);
        }
        fmt++;
    }
    va_end(ap);
}

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

int main(void) {
    olen = 0;
    kprintf("c=%c s=%s d=%d u=%u x=%x %%",
            (int)'Z', "hi", -3LL, 9ULL, 0xabULL);
    obuf[olen] = 0;
    if (!streq(obuf, "c=Z s=hi d=-3 u=9 x=ab %")) return 1;
    /* Second pass: ensure fmt walk still works after prior va_list use. */
    olen = 0;
    kprintf("%s/%d", "ok", 7LL);
    obuf[olen] = 0;
    if (!streq(obuf, "ok/7")) return 2;
    return 42;
}
