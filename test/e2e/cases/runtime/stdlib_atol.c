// runtime.atol: like runtime.atoi but returns long, so it can represent the full 64-bit
// range instead of being truncated to int.  Pin a plain value, a negative,
// a value with leading whitespace and an explicit + sign, and one large
// enough that (int) would wrap but long does not.
// expect: 0
package main;
import runtime;
int main() {
    if (runtime.atol("42") != 42L) return 1;
    if (runtime.atol("-7") != -7L) return 2;
    if (runtime.atol("  123") != 123L) return 3;
    if (runtime.atol("+5") != 5L) return 4;
    if (runtime.atol("0") != 0L) return 5;

    /* 3000000000 overflows 32-bit int but fits in 64-bit long */
    if (runtime.atol("3000000000") != 3000000000L) return 6;

    return 0;
}
