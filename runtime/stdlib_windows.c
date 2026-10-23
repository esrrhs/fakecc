/* Windows/x86-64 runtime pieces.  Selected by the _windows suffix in the
 * package loader: the dialect has no conditional compilation, so per-target
 * code is split by filename.  The x86 instruction selector is the Linux one
 * (SysV), and kernel32 is reached by __syscall numbers the PE linker
 * translates. */
package runtime;

/* stdio.c rewrites `/tmp/...` only when this is set.  CreateFileA does not
 * accept an MSYS path. */
int __fakecc_host_windows = 1;

/* mmap flags.  malloc.c still passes the Linux constants; the Windows
 * translator ignores them and calls VirtualAlloc.  The symbols have to
 * exist because malloc.c and sanitizer.c reference them by name. */
long __fakecc_map_private_anon = 0x0022;
long __fakecc_map_fixed_anon = 0x4032;

/* Microsoft x64 va_list is a pointer (char *): 8 bytes. */
void __fakecc_va_copy(void *dst, void *src) {
    *(void **)dst = *(void **)src;
}

/* libc mmap.  The PE translator turns syscall 9 into VirtualAlloc and ignores
 * the hint address.  Torture ports still need the symbol so they can link;
 * the Win64 cases that only run the body when sizeof(void *)==sizeof(int)
 * return before using the pointer. */
void *mmap(void *addr, unsigned long length, int prot, int flags, int fd,
           long offset) {
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    return (void *)__syscall(9, (long)addr, (long)length, 0, 0, -1, 0);
}

/* chmod has no faithful kernel32 equivalent that matches Unix mode bits.
 * A path that cannot be opened still has to come back as failure: callers
 * distinguish "the file exists" from "it does not". */
extern int open(const char *path, int flags, int mode);
int chmod(const char *path, int mode) {
    int fd;
    (void)mode;
    fd = open(path, 0, 0);
    if (fd < 0) return -1;
    __syscall(3, (long)fd);
    return 0;
}

/* Filled by the PE entry stub with the block from GetEnvironmentStringsA
 * (NAME=value pairs separated by NULs, ended by an extra NUL). */
char *__fakecc_win_env;

char *getenv(const char *name) {
    char *p;
    int nlen;
    int i;
    if (name == 0 || __fakecc_win_env == 0) return 0;
    nlen = 0;
    while (name[nlen]) nlen = nlen + 1;
    if (nlen == 0) return 0;
    p = __fakecc_win_env;
    while (p[0]) {
        i = 0;
        while (i < nlen && p[i] == name[i]) i = i + 1;
        if (i == nlen && p[i] == '=') return p + i + 1;
        while (p[0]) p = p + 1;
        p = p + 1;
    }
    return 0;
}

/* Directory listing for the self-hosted compiler.  Stage 0's pkg.c calls
 * Win32 directly; the translated compiler calls these, and the PE syscall
 * trampoline turns 200/201/202 into FindFirstFileA/FindNextFileA/FindClose.
 * WIN32_FIND_DATAA.cFileName is at byte 44. */
struct win_dir {
    long handle;
    char data[320];
    char pattern[520];
    int have;
};

void *__fakecc_dir_open(const char *path) {
    struct win_dir *d;
    int n;
    long h;
    if (path == 0 || path[0] == 0) return 0;
    d = (struct win_dir *)malloc(sizeof(struct win_dir));
    if (d == 0) return 0;
    n = 0;
    while (path[n] && n < 500) {
        d->pattern[n] = path[n];
        n = n + 1;
    }
    if (n > 0 && d->pattern[n - 1] != '/' && d->pattern[n - 1] != '\\') {
        d->pattern[n] = '\\';
        n = n + 1;
    }
    d->pattern[n] = '*';
    d->pattern[n + 1] = 0;
    h = __syscall(200, (long)d->pattern, (long)d->data);
    if (h == -1) {
        free(d);
        return 0;
    }
    d->handle = h;
    d->have = 1;
    return (void *)d;
}

const char *__fakecc_dir_next(void *h) {
    struct win_dir *d;
    long ok;
    d = (struct win_dir *)h;
    if (d == 0) return 0;
    if (d->have) {
        d->have = 0;
        return (const char *)(d->data + 44);
    }
    ok = __syscall(201, d->handle, (long)d->data);
    if (ok == 0) return 0;
    return (const char *)(d->data + 44);
}

int __fakecc_dir_close(void *h) {
    struct win_dir *d;
    d = (struct win_dir *)h;
    if (d == 0) return -1;
    __syscall(202, d->handle);
    free(h);
    return 0;
}
