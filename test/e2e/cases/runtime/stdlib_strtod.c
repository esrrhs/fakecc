// runtime.strtod: converts a decimal string to a double.  The implementation
// parses integer-valued literals exactly, so pin those: a positive
// skip_oracle: clang
// integer, a negative integer, zero, and a value with an exponent.
// expect: 0
package main;
import runtime;
int main() {
    if ((int)runtime.strtod("100", 0) != 100) return 1;
    if ((int)runtime.strtod("-5", 0) != -5) return 2;
    if ((int)runtime.strtod("0", 0) != 0) return 3;
    if ((int)runtime.strtod("1e3", 0) != 1000) return 4;

    /* endptr stops at the first non-digit */
    char *end;
    runtime.strtod("42abc", &end);
    if (end[0] != 'a') return 5;

    /* 2^63 and a 19-digit odd integer: both are exactly representable, so a
     * correct parse returns them bit for bit.  Folding the digits into a
     * `long double` accumulator loses them on arm64, where long double *is*
     * double and has only a 53-bit significand -- 2^63 came back as
     * 2^63+2^11, which self-hosted fakecc then embedded in its own ast.c
     * literal and kept the bootstrap off its fixed point. */
    if (runtime.strtod("9223372036854775808.0", 0) != 9223372036854775808.0)
        return 6;
    if (runtime.strtod("1234567890123456789.0", 0) != 1234567890123456789.0)
        return 7;

    return 0;
}
