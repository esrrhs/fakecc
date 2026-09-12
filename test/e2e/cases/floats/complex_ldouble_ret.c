// expect: 0
// `_Complex long double` returns in st0 (real) / st1 (imag), not via sret.
package main;
__attribute__((noinline)) _Complex long double make(void) {
    _Complex long double z;
    __real__ z = 5.0L;
    __imag__ z = 6.0L;
    return z;
}
__attribute__((noinline)) _Complex long double id(_Complex long double z) {
    return z;
}
int main(void) {
    _Complex long double z = make();
    if ((int)__real__ z != 5) return 1;
    if ((int)__imag__ z != 6) return 2;
    _Complex long double w = id(z);
    if ((int)__real__ w != 5) return 3;
    if ((int)__imag__ w != 6) return 4;
    return 0;
}
