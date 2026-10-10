/* Darwin runtime pieces, selected by filename (see c_file_matches_target in
 * src/pkg.c).  arm64's va_list is a single pointer -- the variadic area's
 * address -- so the copy is one load and one store.  Copying the x86-64
 * 24 bytes here would read and write past the end of it, which is what made
 * printf's vfprintf jump to a garbage address. */
package runtime;

/* mmap flags for MAP_ANONYMOUS mappings.
 *
 * These are not the Linux values.  Linux gives MAP_ANONYMOUS 0x20 and
 * MAP_NORESERVE 0x4000; Darwin gives 0x1000 and 0x0040.  Passing the Linux
 * 0x22 (MAP_PRIVATE|MAP_ANONYMOUS) therefore asks Darwin for MAP_PRIVATE
 * plus an undefined bit and no MAP_ANON at all, and the mmap fails -- which
 * left the sanitizer with no shadow memory and every access looking like an
 * out-of-bounds one.
 *
 * MAP_FIXED is 0x10 on both, so requesting an exact address still works. */
long __fakecc_map_private_anon = 0x0002 | 0x1000;                    /* malloc */
long __fakecc_map_fixed_anon = 0x0002 | 0x1000 | 0x0010 | 0x0040;   /* asan shadow */

void __fakecc_va_copy(void *dst, void *src) {
    char *d = (char *)dst;
    char *s = (char *)src;
    d[0] = s[0];
    d[1] = s[1];
    d[2] = s[2];
    d[3] = s[3];
    d[4] = s[4];
    d[5] = s[5];
    d[6] = s[6];
    d[7] = s[7];
}

/* ---- Darwin process environment and syscall wrappers ----
 *
 * The LC_MAIN entry stub receives envp from dyld and hands it to
 * __fakecc_set_environ before main runs (see the arm64 backend's entry
 * stub).  getenv scans that vector in place; the returned pointer aliases
 * the process environment, so no static buffer is needed. */

static char **__fakecc_envp = 0;

/* Runs from the entry stub before main, so it must never be instrumented:
 * under -finstrument-functions its entry would otherwise increment the
 * user's __cyg_profile counters before main's first statement. */
__attribute__((no_instrument_function))
void __fakecc_set_environ(char **envp) {
    __fakecc_envp = envp;
}

char *getenv(const char *name) {
    char **e = __fakecc_envp;
    if (e == 0) return 0;
    size_t nlen = 0;
    while (name[nlen]) nlen = nlen + 1;
    while (*e) {
        char *p = *e;
        size_t i = 0;
        while (p[i] && p[i] != '=') i = i + 1;
        if (p[i] == '=' && i == nlen) {
            size_t j = 0;
            while (j < nlen && p[j] == name[j]) j = j + 1;
            if (j == nlen) return p + i + 1;
        }
        e = e + 1;
    }
    return 0;
}

/* chmod/getuid.  The raw __syscall helper speaks Linux x86-64 numbers and
 * the backend maps them to Darwin (chmod 90->15, getuid 102->24), same as
 * every other call in the shared runtime — so use the Linux numbers here
 * too.  The arm64 backend also emits these for bare builtin calls; the
 * runtime exports them so a qualified runtime.chmod/getuid resolves the
 * same way. */
int chmod(const char *path, int mode) {
    long r = __syscall(90, (long)path, (long)mode);
    return r < 0 ? -1 : 0;
}

int getuid(void) {
    return (int)__syscall(102);
}
