/* Minimal FILE stdio over Linux syscalls — FakeCC dialect. */
package runtime;

static FILE _rt_stdin = { 0, 0, 0, 0, 0, 1024, 0 };
static FILE _rt_stdout = { 1, 1, 0, 0, 0, 1024, 0 };
static FILE _rt_stderr = { 2, 1, 0, 0, 0, 1024, 0 };
FILE *stdin = &_rt_stdin;
FILE *stdout = &_rt_stdout;
FILE *stderr = &_rt_stderr;

static int rt_stdio_ready;

void __rt_stdio_init(void) {
    if (rt_stdio_ready) return;
    rt_stdio_ready = 1;
    _rt_stdin.fd = 0;
    _rt_stdin.writable = 0;
    _rt_stdin.buf_cap = 1024;
    _rt_stdout.fd = 1;
    _rt_stdout.writable = 1;
    _rt_stdout.buf_cap = 1024;
    _rt_stderr.fd = 2;
    _rt_stderr.writable = 1;
    _rt_stderr.buf_cap = 1024;
    stdin = &_rt_stdin;
    stdout = &_rt_stdout;
    stderr = &_rt_stderr;
}

static void stdio_init(void) {
    __rt_stdio_init();
}

static FILE *open_files[64];
static int open_files_n;

static void track_fopen(FILE *f) {
    if (open_files_n < 64) {
        open_files[open_files_n] = f;
        open_files_n = open_files_n + 1;
    }
}

static void untrack_fopen(FILE *f) {
    int i = 0;
    while (i < open_files_n) {
        if (open_files[i] == f) {
            open_files[i] = open_files[open_files_n - 1];
            open_files_n = open_files_n - 1;
            return;
        }
        i = i + 1;
    }
}

static int parse_fopen_mode(const char *mode, int *flags, int *writable) {
    int plus = 0;
    int i = 1;
    if (!mode || !mode[0]) return 0;
    while (mode[i]) {
        if (mode[i] == '+') plus = 1;
        i = i + 1;
    }
    *writable = 0;
    if (mode[0] == 'r') {
        *flags = plus ? 2 : 0;
        *writable = plus;
        return 1;
    }
    if (mode[0] == 'w') {
        *flags = (plus ? 2 : 1) | 64 | 512;
        *writable = 1;
        return 1;
    }
    if (mode[0] == 'a') {
        *flags = (plus ? 2 : 1) | 64 | 1024;
        *writable = 1;
        return 1;
    }
    return 0;
}

int fflush(FILE *f) {
    stdio_init();
    if (f == 0) {
        int rc = 0;
        int i = 0;
        if (fflush(stdout) != 0) rc = -1;
        if (fflush(stderr) != 0) rc = -1;
        while (i < open_files_n) {
            if (fflush(open_files[i]) != 0) rc = -1;
            i = i + 1;
        }
        return rc;
    }
    if (!f->writable || f->buf_len == 0) return 0;
    long off = 0;
    while (off < (long)f->buf_len) {
        long n = __syscall(1, (long)f->fd, (long)(f->buf + off),
                           (long)f->buf_len - off);
        if (n <= 0) {
            f->err = 1;
            return -1;
        }
        off = off + n;
    }
    f->buf_len = 0;
    return 0;
}

static int file_write(FILE *f, const char *p, size_t n) {
    stdio_init();
    if (!f->writable) {
        f->err = 1;
        return -1;
    }
    size_t i = 0;
    while (i < n) {
        if (f->buf_len >= f->buf_cap) {
            if (fflush(f) != 0) return -1;
            if (f->buf_len >= f->buf_cap) {
                f->err = 1;
                return -1;
            }
        }
        f->buf[f->buf_len] = p[i];
        f->buf_len = f->buf_len + 1;
        /* Line-buffer stdout on newline; stderr is unbuffered. */
        if (f == stderr) {
            if (fflush(f) != 0) return -1;
        } else if (p[i] == '\n' && f == stdout) {
            if (fflush(f) != 0) return -1;
        }
        i = i + 1;
    }
    return (int)n;
}

int fputc(int c, FILE *f) {
    char ch = (char)c;
    if (file_write(f, &ch, 1) < 0) return -1;
    return (unsigned char)ch;
}

int fputs(const char *s, FILE *f) {
    size_t n = 0;
    while (s[n]) n = n + 1;
    if (file_write(f, s, n) < 0) return -1;
    return 0;
}

int puts(const char *s) {
    stdio_init();
    if (fputs(s, stdout) < 0) return -1;
    if (fputc('\n', stdout) < 0) return -1;
    return 0;
}

int putchar(int c) {
    stdio_init();
    return fputc(c, stdout);
}

size_t fwrite(const void *p, size_t sz, size_t nm, FILE *f) {
    if (sz == 0 || nm == 0) return 0;
    if (nm > ((size_t)-1) / sz) {
        f->err = 1;
        return 0;
    }
    size_t i = 0;
    const char *src = (const char *)p;
    while (i < nm) {
        if (file_write(f, src + i * sz, sz) < 0) return i;
        i = i + 1;
    }
    return nm;
}

size_t fread(void *p, size_t sz, size_t nm, FILE *f) {
    stdio_init();
    if (sz == 0 || nm == 0) return 0;
    if (nm > ((size_t)-1) / sz) {
        f->err = 1;
        return 0;
    }
    size_t total = sz * nm;
    unsigned char *dst = (unsigned char *)p;
    size_t done = 0;
    while (f->nunget > 0 && done < total) {
        f->nunget = f->nunget - 1;
        dst[done] = (unsigned char)f->ungot[f->nunget];
        done = done + 1;
    }
    if (done < total) {
        long n = __syscall(0, (long)f->fd, (long)(dst + done), (long)(total - done));
        if (n < 0) {
            f->err = 1;
            if (done == 0) return 0;
        } else if (n == 0) {
            f->eof = 1;
        } else {
            done = done + (size_t)n;
        }
    }
    return done / sz;
}

FILE *fopen(const char *path, const char *mode) {
    stdio_init();
    int flags = 0;
    int writable = 0;
    if (!parse_fopen_mode(mode, &flags, &writable)) return 0;
    long fd = __syscall(2, (long)path, (long)flags, 420); /* 0644 */
    if (fd < 0) return 0;
    FILE *f = (FILE *)malloc(sizeof(FILE));
    if (f == 0) {
        __syscall(3, fd);
        return 0;
    }
    memset(f, 0, sizeof(FILE));
    f->fd = (int)fd;
    f->writable = writable;
    f->buf_cap = 1024;
    track_fopen(f);
    return f;
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
    stdio_init();
    if (stream == 0) return 0;
    fflush(stream);
    int flags = 0;
    int writable = 0;
    if (!parse_fopen_mode(mode, &flags, &writable)) return 0;
    long fd = __syscall(2, (long)path, (long)flags, 420); /* 0644 */
    if (fd < 0) return 0;
    if (stream->fd >= 0 && stream != stdin && stream != stdout && stream != stderr) {
        __syscall(3, (long)stream->fd);
    }
    stream->fd = (int)fd;
    stream->writable = writable;
    stream->buf_len = 0;
    stream->nunget = 0;
    stream->eof = 0;
    stream->err = 0;
    return stream;
}

int fclose(FILE *f) {
    if (f == 0) return -1;
    fflush(f);
    long r = __syscall(3, (long)f->fd);
    if (f != stdin && f != stdout && f != stderr) {
        untrack_fopen(f);
        free(f);
    }
    return r < 0 ? -1 : 0;
}

int fseek(FILE *f, long off, int whence) {
    stdio_init();
    fflush(f);
    f->nunget = 0;
    long r = __syscall(8, (long)f->fd, off, (long)whence);
    if (r < 0) {
        f->err = 1;
        return -1;
    }
    f->eof = 0;
    return 0;
}

long ftell(FILE *f) {
    stdio_init();
    fflush(f);
    long r = __syscall(8, (long)f->fd, 0, 1); /* SEEK_CUR */
    if (r < 0) {
        f->err = 1;
        return -1;
    }
    return r - (long)f->nunget;
}

int fileno(FILE *f) {
    stdio_init();
    return f->fd;
}

char *tmpnam(char *s) {
    static char buf[64];
    static int seq;
    char *d = s ? s : buf;
    int pid = (int)__syscall(39);
    seq = seq + 1;
    /* /tmp/fcc<pid>_<seq> */
    char *p = d;
    const char *pre = "/tmp/fcc";
    while (*pre) { *p = *pre; p = p + 1; pre = pre + 1; }
    unsigned int v = pid < 0 ? 0 : (unsigned int)pid;
    char tmp[16];
    int n = 0;
    if (v == 0) { tmp[n] = '0'; n = n + 1; }
    while (v > 0 && n < 15) { tmp[n] = (char)('0' + (v % 10)); n = n + 1; v = v / 10; }
    while (n > 0) { n = n - 1; *p = tmp[n]; p = p + 1; }
    *p = '_'; p = p + 1;
    v = (unsigned int)seq;
    n = 0;
    if (v == 0) { tmp[n] = '0'; n = n + 1; }
    while (v > 0 && n < 15) { tmp[n] = (char)('0' + (v % 10)); n = n + 1; v = v / 10; }
    while (n > 0) { n = n - 1; *p = tmp[n]; p = p + 1; }
    *p = '\0';
    return d;
}

int remove(const char *path) {
    long r = __syscall(87, (long)path);
    return r == 0 ? 0 : -1;
}


void perror(const char *s) {
    stdio_init();
    if (s && s[0]) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs("error\n", stderr);
    fflush(stderr);
}

int fgetc(FILE *f) {
    stdio_init();
    if (f->nunget > 0) {
        f->nunget = f->nunget - 1;
        return f->ungot[f->nunget];
    }
    unsigned char ch = 0;
    long n = __syscall(0, (long)f->fd, (long)&ch, 1);
    if (n <= 0) {
        if (n == 0) f->eof = 1;
        else f->err = 1;
        return -1;
    }
    return (int)ch;
}

int ungetc(int c, FILE *f) {
    stdio_init();
    if (c < 0 || f->nunget >= 16) return -1;
    f->ungot[f->nunget] = c;
    f->nunget = f->nunget + 1;
    f->eof = 0;
    return c;
}

int getc(FILE *f) { return fgetc(f); }
int getchar(void) { return fgetc(stdin); }

static int is_space_ch(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static int scan_digit(int ch, int base) {
    int d;
    if (ch >= '0' && ch <= '9') d = ch - '0';
    else if (ch >= 'a' && ch <= 'z') d = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'Z') d = ch - 'A' + 10;
    else return -1;
    if (d >= base) return -1;
    return d;
}

/* sscanf feeds characters from here so we never materialize a stack FILE
 * (fakecc sizeof(FILE) does not include char buf[1024], so a local FILE
 * overlaps the va_list). */
static const char *scan_str;

static int scan_getc(FILE *f) {
    if (scan_str) {
        unsigned char c = (unsigned char)*scan_str;
        if (c == 0) return -1;
        scan_str = scan_str + 1;
        return (int)c;
    }
    return fgetc(f);
}

static void scan_ungetc(int c, FILE *f) {
    if (c < 0) return;
    if (scan_str) {
        scan_str = scan_str - 1;
        return;
    }
    ungetc(c, f);
}

static int scan_letter_eq(int ch, char lo) {
    if (ch >= 'A' && ch <= 'Z') ch = ch + ('a' - 'A');
    return ch == (int)lo;
}

static int scan_take_ch(FILE *f, int *ch, int *used, int *nread, int maxw,
                        char *buf, int *blen, int cap) {
    if (*ch < 0 || *used >= maxw || *blen >= cap - 1) return 0;
    buf[*blen] = (char)(*ch);
    *blen = *blen + 1;
    *used = *used + 1;
    *nread = *nread + 1;
    *ch = scan_getc(f);
    return 1;
}

/* Put lookahead and buf[mark..) back, then reload lookahead. */
static void scan_rewind_to(FILE *f, int *ch, int *used, int *nread,
                           char *buf, int *blen, int mark) {
    if (*ch >= 0) scan_ungetc(*ch, f);
    while (*blen > mark) {
        *blen = *blen - 1;
        scan_ungetc((unsigned char)buf[*blen], f);
        *used = *used - 1;
        *nread = *nread - 1;
    }
    *ch = scan_getc(f);
}

/* Collect a strtod-style subject sequence.  Returns 1 and fills buf on
 * success; on failure restores the first unmatched character to *ch. */
static int scan_collect_fp(FILE *f, int *chp, int *nreadp, int maxw,
                           char *buf, int cap) {
    int ch = *chp;
    int nread = *nreadp;
    int used = 0;
    int blen = 0;
    int any = 0;
    int mark;

    if (ch == '+' || ch == '-') {
        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap))
            goto fail;
    }

    if (scan_letter_eq(ch, 'i')) {
        mark = blen;
        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) goto fail;
        if (!scan_letter_eq(ch, 'n')
            || !scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            scan_rewind_to(f, &ch, &used, &nread, buf, &blen, mark);
            goto fail;
        }
        if (!scan_letter_eq(ch, 'f')
            || !scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            scan_rewind_to(f, &ch, &used, &nread, buf, &blen, mark);
            goto fail;
        }
        if (scan_letter_eq(ch, 'i')) {
            int m2 = blen;
            if (scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)
                && scan_letter_eq(ch, 'n')
                && scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)
                && scan_letter_eq(ch, 'i')
                && scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)
                && scan_letter_eq(ch, 't')
                && scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)
                && scan_letter_eq(ch, 'y')
                && scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
                /* infinity */
            } else {
                scan_rewind_to(f, &ch, &used, &nread, buf, &blen, m2);
            }
        }
        any = 1;
        goto done;
    }

    if (scan_letter_eq(ch, 'n')) {
        mark = blen;
        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) goto fail;
        if (!scan_letter_eq(ch, 'a')
            || !scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            scan_rewind_to(f, &ch, &used, &nread, buf, &blen, mark);
            goto fail;
        }
        if (!scan_letter_eq(ch, 'n')
            || !scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            scan_rewind_to(f, &ch, &used, &nread, buf, &blen, mark);
            goto fail;
        }
        any = 1;
        /* Match glibc scanf: NAN is the subject sequence; a following
         * `(n-char-sequence)` is leftover, unlike C99 strtod. */
        goto done;
    }

    if (ch == '0') {
        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) goto fail;
        any = 1;
        if (ch == 'x' || ch == 'X') {
            int hex_any = 0;
            mark = blen;
            if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) goto done;
            while (scan_digit(ch, 16) >= 0) {
                hex_any = 1;
                if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
            }
            if (ch == '.') {
                if (scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
                    while (scan_digit(ch, 16) >= 0) {
                        hex_any = 1;
                        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
                    }
                }
            }
            if (!hex_any) {
                scan_rewind_to(f, &ch, &used, &nread, buf, &blen, mark);
                goto done;
            }
            if (ch == 'p' || ch == 'P') {
                int pmark = blen;
                if (scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
                    if (ch == '+' || ch == '-')
                        scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap);
                    if (ch >= '0' && ch <= '9') {
                        while (ch >= '0' && ch <= '9') {
                            if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
                        }
                    } else {
                        scan_rewind_to(f, &ch, &used, &nread, buf, &blen, pmark);
                    }
                }
            }
            goto done;
        }
    }

    while (ch >= '0' && ch <= '9') {
        any = 1;
        if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
    }
    if (ch == '.') {
        if (scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            while (ch >= '0' && ch <= '9') {
                any = 1;
                if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
            }
        }
    }
    if (!any) goto fail;
    if (ch == 'e' || ch == 'E') {
        int emark = blen;
        if (scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) {
            if (ch == '+' || ch == '-')
                scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap);
            if (ch >= '0' && ch <= '9') {
                while (ch >= '0' && ch <= '9') {
                    if (!scan_take_ch(f, &ch, &used, &nread, maxw, buf, &blen, cap)) break;
                }
            } else {
                scan_rewind_to(f, &ch, &used, &nread, buf, &blen, emark);
            }
        }
    }

done:
    if (!any) goto fail;
    buf[blen] = '\0';
    *chp = ch;
    *nreadp = nread;
    return 1;

fail:
    scan_rewind_to(f, &ch, &used, &nread, buf, &blen, 0);
    *chp = ch;
    *nreadp = nread;
    return 0;
}

int vfscanf(FILE *f, const char *fmt, va_list ap) {
    stdio_init();
    int matched = 0;
    int input_fail = 0;
    int nread = 0;
    int ch = scan_getc(f);
    while (*fmt) {
        if (is_space_ch((unsigned char)*fmt)) {
            while (is_space_ch((unsigned char)*fmt)) fmt++;
            while (ch >= 0 && is_space_ch(ch)) {
                nread = nread + 1;
                ch = scan_getc(f);
            }
            continue;
        }
        if (*fmt != '%') {
            if (ch < 0) { input_fail = 1; break; }
            if (ch != (unsigned char)*fmt) break;
            nread = nread + 1;
            ch = scan_getc(f);
            fmt++;
            continue;
        }
        fmt++;
        if (*fmt == '%') {
            if (ch < 0) { input_fail = 1; break; }
            if (ch != '%') break;
            nread = nread + 1;
            ch = scan_getc(f);
            fmt++;
            continue;
        }
        int suppress = 0;
        if (*fmt == '*') {
            suppress = 1;
            fmt++;
        }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }
        int hh = 0, h = 0, lmod = 0, ll = 0, Lmod = 0;
        int more = 1;
        while (more) {
            if (*fmt == 'h') {
                fmt++;
                if (*fmt == 'h') { hh = 1; fmt++; }
                else h = 1;
            } else if (*fmt == 'l') {
                fmt++;
                if (*fmt == 'l') { ll = 1; fmt++; }
                else lmod = 1;
            } else if (*fmt == 'z' || *fmt == 't' || *fmt == 'j') {
                lmod = 1;
                fmt++;
            } else if (*fmt == 'L') {
                Lmod = 1;
                fmt++;
            } else {
                more = 0;
            }
        }
        int spec = (unsigned char)*fmt;
        if (spec == 0) break;
        fmt++;

        if (spec == 'n') {
            /* Characters consumed, not including the current lookahead. */
            if (!suppress) {
                if (hh) {
                    char *p = va_arg(ap, char *);
                    *p = (char)nread;
                } else if (h) {
                    short *p = va_arg(ap, short *);
                    *p = (short)nread;
                } else if (ll) {
                    long long *p = va_arg(ap, long long *);
                    *p = (long long)nread;
                } else if (lmod) {
                    long *p = va_arg(ap, long *);
                    *p = (long)nread;
                } else {
                    int *p = va_arg(ap, int *);
                    *p = nread;
                }
            }
            continue;
        }

        int maxw = width > 0 ? width : 0x7fffffff;
        int used = 0;

        if (spec == 's') {
            while (ch >= 0 && is_space_ch(ch)) {
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (ch < 0) { input_fail = 1; break; }
            char *s = 0;
            if (!suppress) s = va_arg(ap, char *);
            int len = 0;
            while (ch >= 0 && !is_space_ch(ch) && len < maxw) {
                if (s) s[len] = (char)ch;
                len = len + 1;
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (s) s[len] = '\0';
            if (!suppress) {
                matched++;
            }
            continue;
        }

        if (spec == 'c') {
            if (width <= 0) maxw = 1;
            if (ch < 0) { input_fail = 1; break; }
            char *p = 0;
            if (!suppress) p = va_arg(ap, char *);
            int len = 0;
            while (ch >= 0 && len < maxw) {
                if (p) p[len] = (char)ch;
                len = len + 1;
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (len == 0) break;
            if (!suppress) {
                matched++;
            }
            continue;
        }

        if (spec == 'e' || spec == 'E' || spec == 'f' || spec == 'F'
            || spec == 'g' || spec == 'G' || spec == 'a' || spec == 'A') {
            while (ch >= 0 && is_space_ch(ch)) {
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (ch < 0) { input_fail = 1; break; }
            char fbuf[512];
            if (!scan_collect_fp(f, &ch, &nread, maxw, fbuf, 512)) {
                if (ch < 0) input_fail = 1;
                break;
            }
            char *endp = fbuf;
            if (fbuf[0] == '+' || fbuf[0] == '-') endp = fbuf + 1;
            long double fval;
            if (scan_letter_eq((unsigned char)*endp, 'i')) {
                fval = 1.0L / 0.0L;
                if (fbuf[0] == '-') fval = -fval;
            } else if (scan_letter_eq((unsigned char)*endp, 'n')) {
                fval = 0.0L / 0.0L;
            } else {
                char *end = 0;
                fval = strtold(fbuf, &end);
                if (end == fbuf) break;
            }
            if (!suppress) {
                if (Lmod || ll) {
                    long double *p = va_arg(ap, long double *);
                    *p = fval;
                } else if (lmod) {
                    double *p = va_arg(ap, double *);
                    *p = (double)fval;
                } else {
                    float *p = va_arg(ap, float *);
                    *p = (float)fval;
                }
                matched++;
            }
            continue;
        }

        if (spec == 'd' || spec == 'i' || spec == 'u' || spec == 'x'
            || spec == 'X' || spec == 'o' || spec == 'p') {
            while (ch >= 0 && is_space_ch(ch)) {
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (ch < 0) { input_fail = 1; break; }
            int sign = 1;
            int saw_sign = 0;
            if (used < maxw && (ch == '-' || ch == '+')) {
                if (ch == '-') sign = -1;
                used = used + 1;
                nread = nread + 1;
                saw_sign = 1;
                ch = scan_getc(f);
            }
            int base = 10;
            if (spec == 'x' || spec == 'X' || spec == 'p') base = 16;
            else if (spec == 'o') base = 8;

            int read_digits = 0;
            unsigned long long val = 0;
            if ((spec == 'x' || spec == 'X' || spec == 'p' || spec == 'i')
                && used < maxw && ch == '0') {
                read_digits = 1;
                used = used + 1;
                nread = nread + 1;
                ch = scan_getc(f);
                if (used < maxw && (ch == 'x' || ch == 'X')) {
                    /* glibc vfscanf consumes the 0x prefix even when no hex
                     * digit follows (`sscanf("0xZ","%x%c")` assigns 'Z').
                     * strtoul is different and still stops at 'x'. */
                    used = used + 1;
                    nread = nread + 1;
                    ch = scan_getc(f);
                    base = 16;
                } else if (spec == 'i') {
                    base = 8;
                }
            }
            while (used < maxw) {
                int d = scan_digit(ch, base);
                if (d < 0) break;
                val = val * (unsigned long long)base + (unsigned long long)d;
                read_digits = 1;
                used = used + 1;
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (!read_digits) {
                /* A lone sign at EOF is a matching failure, not input failure
                 * (C99: sscanf("-", "%d") returns 0, not EOF). */
                if (ch < 0 && !saw_sign) input_fail = 1;
                break;
            }
            if (sign < 0 && spec != 'd' && spec != 'i')
                val = 0ULL - val;
            if (!suppress) {
                if (spec == 'p') {
                    void **p = va_arg(ap, void **);
                    *p = (void *)(unsigned long)val;
                } else if (hh) {
                    char *p = va_arg(ap, char *);
                    if (spec == 'd' || spec == 'i') *p = (char)((long long)val * sign);
                    else *p = (char)val;
                } else if (h) {
                    short *p = va_arg(ap, short *);
                    if (spec == 'd' || spec == 'i') *p = (short)((long long)val * sign);
                    else *p = (short)val;
                } else if (ll) {
                    if (spec == 'd' || spec == 'i') {
                        long long *p = va_arg(ap, long long *);
                        *p = (long long)val * (long long)sign;
                    } else {
                        unsigned long long *p = va_arg(ap, unsigned long long *);
                        *p = val;
                    }
                } else if (lmod) {
                    if (spec == 'd' || spec == 'i') {
                        long *p = va_arg(ap, long *);
                        *p = (long)((long long)val * sign);
                    } else {
                        unsigned long *p = va_arg(ap, unsigned long *);
                        *p = (unsigned long)val;
                    }
                } else {
                    if (spec == 'd' || spec == 'i') {
                        int *p = va_arg(ap, int *);
                        *p = (int)((long long)val * sign);
                    } else {
                        unsigned int *p = va_arg(ap, unsigned int *);
                        *p = (unsigned int)val;
                    }
                }
                matched++;
            }
            continue;
        }

        if (spec == '[') {
            char set[256];
            int si = 0;
            while (si < 256) { set[si] = 0; si = si + 1; }
            int invert = 0;
            if (*fmt == '^') { invert = 1; fmt++; }
            if (*fmt == ']') { set[(unsigned char)']'] = 1; fmt++; }
            int prev = -1;
            while (*fmt && *fmt != ']') {
                unsigned char c = (unsigned char)*fmt;
                fmt++;
                if (prev >= 0 && c == '-' && *fmt && *fmt != ']') {
                    unsigned char e = (unsigned char)*fmt;
                    fmt++;
                    unsigned char lo = (unsigned char)prev;
                    unsigned char hi = e;
                    if (lo > hi) { unsigned char t = lo; lo = hi; hi = t; }
                    while (lo <= hi) {
                        set[lo] = 1;
                        if (lo == 255) break;
                        lo = (unsigned char)(lo + 1);
                    }
                    prev = -1;
                    continue;
                }
                set[c] = 1;
                prev = (int)c;
            }
            if (*fmt == ']') fmt++;
            else break;
            if (ch < 0) { input_fail = 1; break; }
            char *s = 0;
            if (!suppress) s = va_arg(ap, char *);
            int len = 0;
            while (ch >= 0 && len < maxw) {
                int in = set[(unsigned char)ch];
                if (invert) in = !in;
                if (!in) break;
                if (s) s[len] = (char)ch;
                len = len + 1;
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (len == 0) break;
            if (s) s[len] = '\0';
            if (!suppress) matched++;
            continue;
        }
        break;
    }
    scan_ungetc(ch, f);
    if (matched == 0 && input_fail) return -1;
    return matched;
}

int sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    scan_str = s;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    va_end(ap);
    return r;
}

int vsscanf(const char *s, const char *fmt, va_list ap) {
    scan_str = s;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    return r;
}

int __isoc99_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    scan_str = s;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    va_end(ap);
    return r;
}

int fscanf(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfscanf(f, fmt, ap);
    va_end(ap);
    return r;
}

int __isoc99_fscanf(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vfscanf(f, fmt, ap);
    va_end(ap);
    return r;
}

int __isoc99_vfscanf(FILE *f, const char *fmt, va_list ap) {
    return vfscanf(f, fmt, ap);
}
