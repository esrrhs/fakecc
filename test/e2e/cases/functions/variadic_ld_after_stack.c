// expect: 0
// va_arg(long double) after 6 GP registers plus one named stack int: the
// overflow area is 16-aligned, so the caller must leave a hole after the
// stack int.  Same for va_arg(__int128).
package main;
long double pick_ld(int a, int b, int c, int d, int e, int f, int g, ...) {
    va_list ap;
    va_start(ap, g);
    long double ld = va_arg(ap, long double);
    va_end(ap);
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return ld;
}
__int128 pick_i128(int a, int b, int c, int d, int e, int f, int g, ...) {
    va_list ap;
    va_start(ap, g);
    __int128 x = va_arg(ap, __int128);
    va_end(ap);
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return x;
}
int main(void) {
    long double ld = pick_ld(1, 2, 3, 4, 5, 6, 7, 123.0L);
    if ((int)ld != 123) return 1;
    __int128 v = ((__int128)1 << 80) + 99;
    __int128 g = pick_i128(1, 2, 3, 4, 5, 6, 7, v);
    if ((int)g != 99) return 2;
    if ((int)(g >> 80) != 1) return 3;
    return 0;
}
