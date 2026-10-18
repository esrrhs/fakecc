/* Linux/x86-64 runtime pieces.  Selected by the _linux suffix in the
 * package loader (see c_file_matches_target in src/pkg.c): the dialect has
 * no conditional compilation, so per-target code is split by filename. */
package runtime;

/* mmap flags for MAP_ANONYMOUS mappings: the Linux values (see the Darwin
 * file for why these differ per target). */
long __fakecc_map_private_anon = 0x0022;                    /* MAP_PRIVATE|MAP_ANONYMOUS */
long __fakecc_map_fixed_anon = 0x4032;                      /* + MAP_FIXED|MAP_NORESERVE */

/* SysV AMD64 va_list is __va_list_tag[1]: four fields, 24 bytes. */
void __fakecc_va_copy(void *dst, void *src) {
    char *d = (char *)dst;
    char *s = (char *)src;
    int i = 0;
    while (i < 24) {
        d[i] = s[i];
        i = i + 1;
    }
}

/* Linux/x86-64 chmod (syscall 90).  Darwin instead uses the backend's
 * own chmod builtin (syscall 15), so this lives in the Linux-only file. */
int chmod(const char *path, int mode) {
    long r = __syscall(90, (long)path, (long)mode);
    return r < 0 ? -1 : 0;
}

/* ---- Directory listing ----
 *
 * The compiler enumerates a package directory's *.c files, and a
 * self-hosted build has no libc to ask.  Linux spells this getdents64 (217)
 * with a dirent64 record; Darwin spells it getdirentries64 (344) with a
 * different record and a fourth argument, so both live in their own file
 * and the caller only ever sees names. */
struct __fakecc_dirent {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};

struct __fakecc_dir {
    long fd;
    long left;
    long off;
    char buf[2048];
};

void *__fakecc_dir_open(const char *path) {
    /* open(2): O_RDONLY (0) | O_DIRECTORY (0x10000 on Linux).  A plain file
     * fails this, which is what is_dir() relies on. */
    long fd = __syscall(2, (long)path, 0x10000L, 0);
    if (fd < 0) return 0;
    struct __fakecc_dir *d = (struct __fakecc_dir *)malloc(sizeof(struct __fakecc_dir));
    if (d == 0) {
        __syscall(3, fd);
        return 0;
    }
    d->fd = fd;
    d->left = 0;
    d->off = 0;
    return (void *)d;
}

const char *__fakecc_dir_next(void *h) {
    struct __fakecc_dir *d = (struct __fakecc_dir *)h;
    if (d == 0) return 0;
    while (1) {
        if (d->off < d->left) {
            struct __fakecc_dirent *e =
                (struct __fakecc_dirent *)(void *)(d->buf + d->off);
            long reclen = (long)e->d_reclen;
            if (reclen <= 0) return 0;
            d->off = d->off + reclen;
            return (const char *)e->d_name;
        }
        d->left = 0;
        d->off = 0;
        long n = __syscall(217, d->fd, (long)d->buf, 2048);
        if (n <= 0) return 0;
        d->left = n;
    }
}

int __fakecc_dir_close(void *h) {
    struct __fakecc_dir *d = (struct __fakecc_dir *)h;
    if (d == 0) return -1;
    __syscall(3, d->fd);
    free(h);
    return 0;
}

/* Scan /proc/self/environ.  Returned pointer is into a static buffer.
 * Darwin's getenv is an arm64 backend builtin walking the entry envp. */
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
