// strtod: converts a decimal string to a double.  The implementation
// parses integer-valued literals exactly, so pin those: a positive
// integer, a negative integer, zero, and a value with an exponent.
// expect: 0
package main;
extern double strtod(const char *s, char **end);
int main() {
    if ((int)strtod("100", 0) != 100) return 1;
    if ((int)strtod("-5", 0) != -5) return 2;
    if ((int)strtod("0", 0) != 0) return 3;
    if ((int)strtod("1e3", 0) != 1000) return 4;

    /* endptr stops at the first non-digit */
    char *end;
    strtod("42abc", &end);
    if (end[0] != 'a') return 5;

    return 0;
}
