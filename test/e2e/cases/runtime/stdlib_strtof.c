// runtime.strtof: converts a decimal string to a float.  Like runtime.strtod
// but returns float.  The implementation parses integer-valued literals
// exactly, so pin those (cast to int for an exact comparison), a negative,
// zero, an exponent, and endptr advancement.
// expect: 0
package main;
import runtime;
int main() {
    if ((int)runtime.strtof("100", 0) != 100) return 1;
    if ((int)runtime.strtof("-5", 0) != -5) return 2;
    if ((int)runtime.strtof("0", 0) != 0) return 3;
    if ((int)runtime.strtof("1e3", 0) != 1000) return 4;

    /* endptr stops at the first non-digit */
    char *end;
    runtime.strtof("42abc", &end);
    if (end[0] != 'a') return 5;

    /* 2^24 is exactly representable in float (23-bit mantissa + hidden bit) */
    float f = runtime.strtof("16777216", 0);
    if (f != 16777216.0f) return 6;

    return 0;
}
