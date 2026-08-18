/* Minimal FILE stdio over Linux syscalls — FakeCC dialect. */
package runtime;

static FILE _rt_stdin = { 0, 0, 0, 0, 0, 1024 };
static FILE _rt_stdout = { 1, 1, 0, 0, 0, 1024 };
static FILE _rt_stderr = { 2, 1, 0, 0, 0, 1024 };
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
    char *d = s ? s : buf;
    strcpy(d, "/tmp/fccXXXXXX");
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
