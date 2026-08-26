/* Minimal FILE stdio over Linux syscalls — FakeCC dialect. */
package runtime;

static FILE _rt_stdin = { 0, 0, 0, 0, 0, 1024, 0, 0 };
static FILE _rt_stdout = { 1, 1, 0, 0, 0, 1024, 0, 0 };
static FILE _rt_stderr = { 2, 1, 0, 0, 0, 1024, 0, 0 };
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

int fflush(FILE *f) {
    stdio_init();
    if (f == 0) {
        fflush(stdout);
        fflush(stderr);
        return 0;
    }
    if (!f->writable || f->buf_len == 0) return 0;
    long n = __syscall(1, (long)f->fd, (long)f->buf, (long)f->buf_len);
    if (n < 0) {
        f->err = 1;
        return -1;
    }
    f->buf_len = 0;
    return 0;
}

static int file_write(FILE *f, const char *p, size_t n) {
    stdio_init();
    size_t i = 0;
    while (i < n) {
        if (f->buf_len >= f->buf_cap) {
            if (fflush(f) != 0) return -1;
        }
        f->buf[f->buf_len] = p[i];
        f->buf_len = f->buf_len + 1;
        /* Line-buffer stdout/stderr on newline. */
        if (p[i] == '\n' && (f == stdout || f == stderr)) {
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
    size_t total = sz * nm;
    if (file_write(f, (const char *)p, total) < 0) return 0;
    return nm;
}

size_t fread(void *p, size_t sz, size_t nm, FILE *f) {
    stdio_init();
    size_t total = sz * nm;
    long n = __syscall(0, (long)f->fd, (long)p, (long)total);
    if (n <= 0) {
        if (n == 0) f->eof = 1;
        else f->err = 1;
        return 0;
    }
    return (size_t)n / sz;
}

FILE *fopen(const char *path, const char *mode) {
    stdio_init();
    int flags = 0;
    int writable = 0;
    if (mode[0] == 'r') {
        flags = 0; /* O_RDONLY */
        if (mode[1] == '+') {
            flags = 2; /* O_RDWR */
            writable = 1;
        }
    } else if (mode[0] == 'w') {
        flags = 1 | 64 | 512; /* O_WRONLY|O_CREAT|O_TRUNC */
        writable = 1;
        if (mode[1] == 'b' && mode[2] == '+') flags = 2 | 64 | 512;
        else if (mode[1] == '+') flags = 2 | 64 | 512;
    } else if (mode[0] == 'a') {
        flags = 1 | 64 | 1024; /* O_WRONLY|O_CREAT|O_APPEND */
        writable = 1;
    } else {
        return 0;
    }
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
    return f;
}

FILE *freopen(const char *path, const char *mode, FILE *stream) {
    stdio_init();
    if (stream == 0) return 0;
    fflush(stream);
    int flags = 0;
    int writable = 0;
    if (mode[0] == 'r') {
        flags = 0; /* O_RDONLY */
        if (mode[1] == '+') {
            flags = 2; /* O_RDWR */
            writable = 1;
        }
    } else if (mode[0] == 'w') {
        flags = 1 | 64 | 512; /* O_WRONLY|O_CREAT|O_TRUNC */
        writable = 1;
        if (mode[1] == 'b' && mode[2] == '+') flags = 2 | 64 | 512;
        else if (mode[1] == '+') flags = 2 | 64 | 512;
    } else if (mode[0] == 'a') {
        flags = 1 | 64 | 1024; /* O_WRONLY|O_CREAT|O_APPEND */
        writable = 1;
    } else {
        return 0;
    }
    long fd = __syscall(2, (long)path, (long)flags, 420); /* 0644 */
    if (fd < 0) return 0;
    if (stream->fd >= 0 && stream != stdin && stream != stdout && stream != stderr) {
        __syscall(3, (long)stream->fd);
    }
    stream->fd = (int)fd;
    stream->writable = writable;
    stream->buf_len = 0;
    stream->has_ungot = 0;
    stream->ungot = 0;
    stream->eof = 0;
    stream->err = 0;
    return stream;
}

int fclose(FILE *f) {
    if (f == 0) return -1;
    fflush(f);
    long r = __syscall(3, (long)f->fd);
    if (f != stdin && f != stdout && f != stderr) free(f);
    return r < 0 ? -1 : 0;
}

int fseek(FILE *f, long off, int whence) {
    stdio_init();
    fflush(f);
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
    return r;
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
    return (int)__syscall(87, (long)path);
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
    if (f->has_ungot) {
        f->has_ungot = 0;
        return f->ungot;
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
    if (c < 0 || f->has_ungot) return -1;
    f->has_ungot = 1;
    f->ungot = c;
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
static int scan_str_has;
static int scan_str_ungot;

static int scan_getc(FILE *f) {
    if (scan_str) {
        if (scan_str_has) {
            scan_str_has = 0;
            return scan_str_ungot;
        }
        unsigned char c = (unsigned char)*scan_str;
        if (c == 0) return -1;
        scan_str = scan_str + 1;
        return (int)c;
    }
    return fgetc(f);
}

static void scan_ungetc(int c, FILE *f) {
    if (scan_str) {
        if (c < 0 || scan_str_has) return;
        scan_str_has = 1;
        scan_str_ungot = c;
        return;
    }
    if (c >= 0) ungetc(c, f);
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
        int hh = 0, h = 0, lmod = 0, ll = 0;
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

        if (spec == 'd' || spec == 'i' || spec == 'u' || spec == 'x'
            || spec == 'X' || spec == 'o' || spec == 'p') {
            while (ch >= 0 && is_space_ch(ch)) {
                nread = nread + 1;
                ch = scan_getc(f);
            }
            if (ch < 0) { input_fail = 1; break; }
            int sign = 1;
            if (spec != 'p' && used < maxw && (ch == '-' || ch == '+')) {
                if (ch == '-') sign = -1;
                used = used + 1;
                nread = nread + 1;
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
                    used = used + 1;
                    nread = nread + 1;
                    ch = scan_getc(f);
                    if (scan_digit(ch, 16) >= 0) base = 16;
                    else {
                        /* "0x" with no hex digit: value 0, lookahead is after x */
                    }
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
                if (ch < 0) input_fail = 1;
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
    scan_str_has = 0;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    scan_str_has = 0;
    va_end(ap);
    return r;
}

int vsscanf(const char *s, const char *fmt, va_list ap) {
    scan_str = s;
    scan_str_has = 0;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    scan_str_has = 0;
    return r;
}

int __isoc99_sscanf(const char *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    scan_str = s;
    scan_str_has = 0;
    int r = vfscanf(stdin, fmt, ap);
    scan_str = 0;
    scan_str_has = 0;
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
