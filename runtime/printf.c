/* printf family — FakeCC dialect.
 * Supports %s %c %d %i %u %x %X %p %f %F %e %E %g %G %%, the l/ll length
 * modifiers, field width and precision.
 *
 * Floating point is formatted from an 18-significant-digit decimal expansion
 * computed in long double, which covers the 17 digits a double carries.
 * Asking for more digits than that (e.g. %.25f, or %f on 1e300) prints the
 * extra positions as the expansion happens to end, rather than the exact
 * binary value glibc would show. */
package runtime;

extern void *__fakecc_va_copy(void *dst, void *src);

static void ensure_stdio(void) {
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

/* Digits of `v` in `base` into out[]; returns the length written. */
static int uint_to_buf(char *out, unsigned long long v, int base, int upper) {
    char tmp[32];
    int n = 0;
    if (v == 0) { tmp[0] = '0'; n = 1; }
    else {
        while (v > 0) {
            int d = (int)(v % (unsigned long long)base);
            v = v / (unsigned long long)base;
            if (d < 10) tmp[n] = (char)('0' + d);
            else if (upper) tmp[n] = (char)('A' + d - 10);
            else tmp[n] = (char)('a' + d - 10);
            n = n + 1;
        }
    }
    int i = 0;
    while (i < n) { out[i] = tmp[n - 1 - i]; i = i + 1; }
    return n;
}

/* Decompose a finite, strictly positive value into its leading decimal digits:
 * on return digits[0..n-1] are ASCII and the value is digits * 10^(*exp10).
 * The scaling runs in long double so all 17 digits a double can carry survive
 * it. */
static int decimal_digits(long double a, char *digits, int *exp10) {
    int e = 0;
    /* Only powers of ten that a double holds exactly appear here: this file is
     * compiled by fakecc itself during the bootstrap, and a literal that has
     * to be rounded would land on different bits depending on whether the
     * compiling compiler used the host strtold or rt's. */
    while (a >= 1.0e18L) { a = a / 1.0e18L; e = e + 18; }
    while (a < 1.0L)     { a = a * 1.0e18L; e = e - 18; }
    while (a >= 1.0e8L)  { a = a / 1.0e8L;  e = e + 8; }
    while (a >= 1.0e2L)  { a = a / 1.0e2L;  e = e + 2; }
    while (a >= 1.0e1L)  { a = a / 1.0e1L;  e = e + 1; }
    /* a is now in [1, 10): lift it to an 18-digit integer.  A double needs 17
     * digits to be identified; the 18th only has to be good enough to tell a
     * true tie (which stays exactly zero here) from a value just off one. */
    a = a * 1.0e17L;
    e = e - 17;
    unsigned long long u = (unsigned long long)a;
    /* Whatever is left below the 18th digit still decides ties, so carry it as
     * one more digit rather than dropping it. */
    long double frac = a - (long double)u;
    int extra = (int)(frac * 10.0L + 0.5L);
    if (extra > 9) extra = 9;
    if (extra == 0 && frac > 0.0L) extra = 1;
    u = u * 10 + (unsigned long long)extra;
    e = e - 1;
    char tmp[32];
    int n = 0;
    while (u > 0) {
        tmp[n] = (char)('0' + (int)(u % 10));
        u = u / 10;
        n = n + 1;
    }
    int i = 0;
    while (i < n) {
        digits[i] = tmp[n - 1 - i];
        i = i + 1;
    }
    *exp10 = e;
    return n;
}

/* Drop the low `k` digits of the expansion.  Ties (a dropped tail of exactly
 * 5000...) go to even, which is what the default FP rounding mode gives and
 * hence what glibc prints; everything else rounds to nearest.
 * Returns the new digit count; *exp10 and digits[] are updated in place. */
static int round_digits(char *digits, int ndig, int *exp10, int k, int min_exp) {
    if (k >= ndig) {
        /* Everything is dropped; only a tail above one half can carry into the
         * (implicit, and therefore even) last kept position. */
        int up = 0;
        if (k == ndig && digits[0] >= '5') {
            up = 1;
            if (digits[0] == '5') {
                int j = 1;
                up = 0;
                while (j < ndig) { if (digits[j] != '0') { up = 1; break; } j = j + 1; }
            }
        }
        digits[0] = (char)(up ? '1' : '0');
        *exp10 = min_exp;
        return 1;
    }
    int first = digits[ndig - k];
    int roundup;
    if (first > '5') roundup = 1;
    else if (first < '5') roundup = 0;
    else {
        int rest = 0;
        int j = ndig - k + 1;
        while (j < ndig) { if (digits[j] != '0') { rest = 1; break; } j = j + 1; }
        if (rest) roundup = 1;
        else roundup = ((digits[ndig - k - 1] - '0') % 2) != 0;  /* tie → even */
    }
    ndig = ndig - k;
    *exp10 = *exp10 + k;
    if (roundup) {
        int i = ndig - 1;
        while (i >= 0) {
            if (digits[i] == '9') { digits[i] = '0'; i = i - 1; }
            else { digits[i] = (char)(digits[i] + 1); break; }
        }
        if (i < 0) {
            /* All nines: 999 + 1 = 1000.  The extra digit widens the
             * expansion; the exponent it is scaled by does not change. */
            int j = ndig;
            while (j > 0) { digits[j] = digits[j - 1]; j = j - 1; }
            digits[0] = '1';
            ndig = ndig + 1;
        }
    }
    return ndig;
}

/* Format `a` (non-negative, finite) as %f with `prec` digits after the point.
 * Returns the length written to out. */
static int fmt_fixed(char *out, long double a, int prec) {
    char digits[40];
    int ndig;
    int e10;
    if (a == 0.0L) { digits[0] = '0'; ndig = 1; e10 = -prec; }
    else {
        ndig = decimal_digits(a, digits, &e10);
        int k = -prec - e10;
        if (k > 0) ndig = round_digits(digits, ndig, &e10, k, -prec);
    }
    int n = 0;
    int frac_have = (e10 < 0) ? -e10 : 0;   /* digits sitting after the point */
    int int_have = ndig - frac_have;        /* digits sitting before it */
    if (int_have <= 0) {
        out[n] = '0'; n = n + 1;
    } else {
        int i = 0;
        while (i < int_have) { out[n] = digits[i]; n = n + 1; i = i + 1; }
        int z = 0;
        while (z < e10) { out[n] = '0'; n = n + 1; z = z + 1; }
    }
    if (prec > 0) {
        out[n] = '.'; n = n + 1;
        int emitted = 0;
        /* Leading zeros for a value smaller than 0.1. */
        int lead = (int_have < 0) ? -int_have : 0;
        while (emitted < lead && emitted < prec) { out[n] = '0'; n = n + 1; emitted = emitted + 1; }
        int i = (int_have > 0) ? int_have : 0;
        while (i < ndig && emitted < prec) {
            out[n] = digits[i]; n = n + 1; i = i + 1; emitted = emitted + 1;
        }
        while (emitted < prec) { out[n] = '0'; n = n + 1; emitted = emitted + 1; }
    }
    return n;
}

/* Format `a` (non-negative, finite) as %e with `prec` digits after the point. */
static int fmt_sci(char *out, long double a, int prec, int upper) {
    char digits[40];
    int ndig;
    int e10;
    if (a == 0.0L) { digits[0] = '0'; ndig = 1; e10 = 0; }
    else {
        ndig = decimal_digits(a, digits, &e10);
        int keep = prec + 1;
        if (ndig > keep) {
            int before = ndig;
            ndig = round_digits(digits, ndig, &e10, ndig - keep, e10 + (before - keep));
        }
    }
    int exp = (a == 0.0L) ? 0 : (e10 + ndig - 1);
    int n = 0;
    out[n] = digits[0]; n = n + 1;
    if (prec > 0) {
        out[n] = '.'; n = n + 1;
        int i = 1;
        int emitted = 0;
        while (i < ndig && emitted < prec) { out[n] = digits[i]; n = n + 1; i = i + 1; emitted = emitted + 1; }
        while (emitted < prec) { out[n] = '0'; n = n + 1; emitted = emitted + 1; }
    }
    out[n] = upper ? 'E' : 'e'; n = n + 1;
    int ex = exp;
    if (ex < 0) { out[n] = '-'; n = n + 1; ex = -ex; }
    else { out[n] = '+'; n = n + 1; }
    if (ex >= 100) {
        out[n] = (char)('0' + ex / 100); n = n + 1;
        out[n] = (char)('0' + (ex / 10) % 10); n = n + 1;
        out[n] = (char)('0' + ex % 10); n = n + 1;
    } else {
        out[n] = (char)('0' + ex / 10); n = n + 1;
        out[n] = (char)('0' + ex % 10); n = n + 1;
    }
    return n;
}

/* Format `a` as %g: %e when the exponent is far from zero, %f otherwise, with
 * trailing zeros removed (C99 7.19.6.1). */
static int fmt_gen(char *out, long double a, int prec, int upper) {
    if (prec == 0) prec = 1;
    int exp;
    if (a == 0.0L) exp = 0;
    else {
        char digits[40];
        int e10;
        int ndig = decimal_digits(a, digits, &e10);
        int keep = prec;
        if (ndig > keep) {
            int before = ndig;
            ndig = round_digits(digits, ndig, &e10, ndig - keep, e10 + (before - keep));
        }
        exp = e10 + ndig - 1;
    }
    int n;
    if (exp < -4 || exp >= prec) n = fmt_sci(out, a, prec - 1, upper);
    else n = fmt_fixed(out, a, prec - 1 - exp);
    /* Strip trailing zeros in the fraction (and a bare trailing point). */
    int dot = -1;
    int i = 0;
    while (i < n) { if (out[i] == '.') { dot = i; break; } i = i + 1; }
    if (dot < 0) return n;
    int end = n;
    int estart = n;
    i = dot;
    while (i < n) { if (out[i] == 'e' || out[i] == 'E') { estart = i; break; } i = i + 1; }
    end = estart;
    while (end > dot + 1 && out[end - 1] == '0') end = end - 1;
    if (end == dot + 1) end = dot;
    if (estart < n) {
        int j = estart;
        while (j < n) { out[end] = out[j]; end = end + 1; j = j + 1; }
    }
    return end;
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
        int f_minus = 0, f_zero = 0, f_plus = 0, f_space = 0, f_alt = 0;
        int flagging = 1;
        while (flagging) {
            if (fmt[i] == '-') f_minus = 1;
            else if (fmt[i] == '0') f_zero = 1;
            else if (fmt[i] == '+') f_plus = 1;
            else if (fmt[i] == ' ') f_space = 1;
            else if (fmt[i] == '#') f_alt = 1;
            else flagging = 0;
            if (flagging) i = i + 1;
        }
        if (fmt[i] == '*') {
            width = va_arg(ap, int);
            if (width < 0) { f_minus = 1; width = -width; }
            i = i + 1;
        } else {
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                width = width * 10 + (fmt[i] - '0');
                i = i + 1;
            }
        }
        if (fmt[i] == '.') {
            i = i + 1;
            precision = 0;
            if (fmt[i] == '*') {
                precision = va_arg(ap, int);
                i = i + 1;
            } else {
                while (fmt[i] >= '0' && fmt[i] <= '9') {
                    precision = precision * 10 + (fmt[i] - '0');
                    i = i + 1;
                }
            }
        }
        /* Length modifiers.  Only l/ll change how the argument is read; the
         * rest are accepted and ignored because default promotion already
         * gives the right register contents. */
        int lenning = 1;
        while (lenning) {
            if (fmt[i] == 'l') { long_count = long_count + 1; i = i + 1; }
            else if (fmt[i] == 'h' || fmt[i] == 'z' || fmt[i] == 'j'
                     || fmt[i] == 't' || fmt[i] == 'L') i = i + 1;
            else lenning = 0;
        }
        char spec = fmt[i];
        if (spec == 0) break;
        i = i + 1;

        /* Every conversion builds its text in body[] plus an optional prefix
         * (sign, or 0x), then goes through one padding path so the flags,
         * width and precision behave the same everywhere. */
        char body[512];
        char prefix[4];
        int blen = 0;
        int plen = 0;
        int zero_ok = 0;    /* '0' flag applies (numeric, no explicit precision) */
        const char *sbody = 0;   /* %s prints from the argument, not body[] */

        if (spec == 's') {
            sbody = va_arg(ap, char *);
            if (sbody == 0) sbody = "(null)";
            blen = (int)strlen(sbody);
            if (precision >= 0 && blen > precision) blen = precision;
        } else if (spec == 'c') {
            body[0] = (char)va_arg(ap, int);
            blen = 1;
        } else if (spec == 'd' || spec == 'i') {
            long long v;
            if (long_count >= 2) v = va_arg(ap, long long);
            else if (long_count == 1) v = (long long)va_arg(ap, long);
            else v = (long long)va_arg(ap, int);
            unsigned long long u;
            if (v < 0) { prefix[0] = '-'; plen = 1; u = (unsigned long long)(-v); }
            else {
                u = (unsigned long long)v;
                if (f_plus) { prefix[0] = '+'; plen = 1; }
                else if (f_space) { prefix[0] = ' '; plen = 1; }
            }
            blen = uint_to_buf(body, u, 10, 0);
            zero_ok = (precision < 0);
        } else if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'o'
                   || spec == 'p') {
            unsigned long long v;
            int base = 10;
            int upper = 0;
            if (spec == 'x') base = 16;
            else if (spec == 'X') { base = 16; upper = 1; }
            else if (spec == 'o') base = 8;
            if (spec == 'p') {
                base = 16;
                prefix[0] = '0'; prefix[1] = 'x'; plen = 2;
                v = (unsigned long long)(unsigned long)va_arg(ap, void *);
            } else if (long_count >= 2) v = va_arg(ap, unsigned long long);
            else if (long_count == 1) v = (unsigned long long)va_arg(ap, unsigned long);
            else v = (unsigned long long)va_arg(ap, unsigned int);
            if (f_alt && v != 0 && (spec == 'x' || spec == 'X')) {
                prefix[0] = '0'; prefix[1] = spec; plen = 2;
            }
            blen = uint_to_buf(body, v, base, upper);
            zero_ok = (precision < 0);
        } else if (spec == 'f' || spec == 'F' || spec == 'e' || spec == 'E'
                   || spec == 'g' || spec == 'G') {
            double dv = va_arg(ap, double);
            /* The sign comes from the bit, not a comparison, so that -0.0 and
             * a negative NaN print with their '-' the way C requires. */
            unsigned long long dbits;
            memcpy((void *)&dbits, (void *)&dv, 8);
            int neg = (dbits >> 63) != 0;
            long double a = (long double)dv;
            if (neg) a = -a;
            /* NaN and infinity have no decimal expansion; C prints them by
             * name.  NaN is the only value that compares unequal to itself. */
            if (dv != dv) {
                body[0] = 'n'; body[1] = 'a'; body[2] = 'n'; blen = 3;
            } else if (dv - dv != 0.0) {   /* only infinities differ from themselves */
                body[0] = 'i'; body[1] = 'n'; body[2] = 'f'; blen = 3;
            } else {
                if (spec == 'f' || spec == 'F')
                    blen = fmt_fixed(body, a, precision < 0 ? 6 : precision);
                else if (spec == 'e' || spec == 'E')
                    blen = fmt_sci(body, a, precision < 0 ? 6 : precision,
                                   spec == 'E');
                else
                    blen = fmt_gen(body, a, precision < 0 ? 6 : precision,
                                   spec == 'G');
                zero_ok = 1;
            }
            if (neg) { prefix[0] = '-'; plen = 1; }
            else if (f_plus) { prefix[0] = '+'; plen = 1; }
            else if (f_space) { prefix[0] = ' '; plen = 1; }
        } else {
            body[0] = '%';
            body[1] = spec;
            blen = 2;
        }

        /* Integer conversions zero-extend the digits to the precision. */
        int lead_zeros = 0;
        if (precision >= 0 && (spec == 'd' || spec == 'i' || spec == 'u'
                               || spec == 'x' || spec == 'X' || spec == 'o')
            && blen < precision)
            lead_zeros = precision - blen;

        int total = plen + lead_zeros + blen;
        int padding = width > total ? width - total : 0;
        if (!f_minus && padding > 0 && f_zero && zero_ok) {
            lead_zeros = lead_zeros + padding;
            padding = 0;
        }
        if (!f_minus && padding > 0) {
            if (pad(&bp, &left, &count, 0, padding, ' ') < 0) return -1;
        }
        int k = 0;
        while (k < plen) {
            if (emit_ch(&bp, &left, &count, 0, prefix[k]) < 0) return -1;
            k = k + 1;
        }
        if (pad(&bp, &left, &count, 0, lead_zeros, '0') < 0) return -1;
        k = 0;
        while (k < blen) {
            char ch = sbody ? sbody[k] : body[k];
            if (emit_ch(&bp, &left, &count, 0, ch) < 0) return -1;
            k = k + 1;
        }
        if (f_minus && padding > 0) {
            if (pad(&bp, &left, &count, 0, padding, ' ') < 0) return -1;
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
    ensure_stdio();
    /* Size then emit — simple two-pass using a stack buffer for small, heap for large. */
    va_list ap2;
    __fakecc_va_copy((void *)&ap2, (void *)&ap);
    int need = vsnprintf(0, 0, fmt, ap2);
    va_end(ap2);
    if (need < 0) return -1;
    char stack[512];
    char *outbuf = stack;
    int heap = 0;
    if (need + 1 > 512) {
        outbuf = (char *)malloc((size_t)need + 1);
        if (outbuf == 0) return -1;
        heap = 1;
    }
    vsnprintf(outbuf, (size_t)need + 1, fmt, ap);
    int i = 0;
    while (i < need) {
        if (fputc((unsigned char)outbuf[i], f) < 0) {
            if (heap) free(outbuf);
            return -1;
        }
        i = i + 1;
    }
    if (heap) free(outbuf);
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
