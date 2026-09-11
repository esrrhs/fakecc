// expect: 0
// va_arg(ap, __int128) reads both eightbytes of a 128-bit integer argument.
package main;
__int128 pick(int n, ...) {
    va_list ap;
    va_start(ap, n);
    __int128 x = va_arg(ap, __int128);
    va_end(ap);
    return x;
}
int main(void) {
    __int128 v = ((__int128)1 << 80) + 99;
    __int128 g = pick(1, v);
    if ((int)g != 99) return 1;
    if ((int)(g >> 80) != 1) return 2;
    return 0;
}
