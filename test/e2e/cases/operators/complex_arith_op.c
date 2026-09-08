// expect: 0
package main;

__complex__ double neg_c(__complex__ double z) {
    return -z;
}

__complex__ double choose_c(int cond, __complex__ double a, double b) {
    // Ternary mixing complex and real (b promoted to complex with imag = 0)
    return cond ? a : b;
}

int main() {
    // 1. Unary negation on _Complex
    __complex__ double c;
    __real__ c = 2.5;
    __imag__ c = 6.5;

    __complex__ double n = -c;
    if (__real__ n != -2.5) return 1;
    if (__imag__ n != -6.5) return 2;

    // 2. Unary positive
    __complex__ double p = +c;
    if (__real__ p != 2.5) return 3;
    if (__imag__ p != 6.5) return 4;

    // 3. Unary neg via function
    __complex__ double fn_neg = neg_c(c);
    if (__real__ fn_neg != -2.5 || __imag__ fn_neg != -6.5) return 5;

    // 4. Ternary operator with real condition and complex branch
    __complex__ double res1 = choose_c(1, c, 10.0);
    if (__real__ res1 != 2.5 || __imag__ res1 != 6.5) return 6;

    // 5. Ternary operator selecting real branch promoted to complex
    __complex__ double res2 = choose_c(0, c, 10.0);
    if (__real__ res2 != 10.0 || __imag__ res2 != 0.0) return 7;

    // 6. Direct inline ternary mixing scalar and complex
    int flag = 1;
    __complex__ double direct1 = flag ? 5.0 : c;
    if (__real__ direct1 != 5.0 || __imag__ direct1 != 0.0) return 8;

    flag = 0;
    __complex__ double direct2 = flag ? 5.0 : c;
    if (__real__ direct2 != 2.5 || __imag__ direct2 != 6.5) return 9;

    // 7. Negation of ternary result
    __complex__ double base3;
    __real__ base3 = 3.0;
    __imag__ base3 = 4.0;
    __complex__ double neg_tern = -(flag ? c : base3);
    if (__real__ neg_tern != -3.0 || __imag__ neg_tern != -4.0) return 10;

    return 0;
}
