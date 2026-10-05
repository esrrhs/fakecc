/* Linux/x86-64 runtime pieces.  Selected by the _linux suffix in the
 * package loader (see c_file_matches_target in src/pkg.c): the dialect has
 * no conditional compilation, so per-target code is split by filename. */
package runtime;

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
