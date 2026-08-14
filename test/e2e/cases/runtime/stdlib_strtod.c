// runtime.strtod: converts a decimal string to a double.  The implementation
// parses integer-valued literals exactly, so pin those: a positive
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

    return 0;
}
