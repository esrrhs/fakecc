// std.strtold: like std.strtod but returns long double.  Pin integer-valued
// literals (parsed exactly), a negative, zero, an exponent, and endptr
// advancement.  Casting to int keeps the assertions exact without
// relying on long-double printing.
// expect: 0
package main;
import std;
int main() {
    if ((int)std.strtold("100", 0) != 100) return 1;
    if ((int)std.strtold("-5", 0) != -5) return 2;
    if ((int)std.strtold("0", 0) != 0) return 3;
    if ((int)std.strtold("1e3", 0) != 1000) return 4;

    /* endptr stops at the first non-digit */
    char *end;
    std.strtold("42abc", &end);
    if (end[0] != 'a') return 5;

    /* a value a double would round, but long double holds exactly:
       10^20 is exactly representable in 80-bit extended (64-bit mantissa) */
    long double big = std.strtold("100000000000000000000", 0);
    if (big != 1e20L) return 6;

    return 0;
}
