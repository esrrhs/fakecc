#ifndef FAKEINC_STDARG_H
#define FAKEINC_STDARG_H
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
/* va_copy on amd64: va_list = __va_list_tag[1] = 24 bytes.
 * Provide as a helper function call that fakecc can compile. */
extern void __fakecc_va_copy(void *dst, void *src);
#define va_copy(dst, src) __fakecc_va_copy((dst), (src))
#endif
