/* Darwin runtime pieces, selected by filename (see c_file_matches_target in
 * src/pkg.c).  arm64's va_list is a single pointer -- the variadic area's
 * address -- so the copy is one load and one store.  Copying the x86-64
 * 24 bytes here would read and write past the end of it, which is what made
 * printf's vfprintf jump to a garbage address. */
package runtime;

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
