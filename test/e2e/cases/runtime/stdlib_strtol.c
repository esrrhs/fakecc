// runtime.strtol: converts a string to a long in a given base, reporting where
// parsing stopped through an endptr.  Pin base-10, sign, endptr, hex,
// the base==0 auto-detect path, and glibc overflow saturation (LONG_MAX /
// LONG_MIN, errno ERANGE=34).
// expect: 0
package main;
import runtime;
int main() {
    char *end;

    if (runtime.strtol("123", 0, 10) != 123) return 1;
    if (runtime.strtol("-42", 0, 10) != -42) return 2;

    /* endptr points at the first non-digit character */
    long v = runtime.strtol("12abc", &end, 10);
    if (v != 12) return 3;
    if (end[0] != 'a') return 4;

    /* explicit hex */
    if (runtime.strtol("0xff", 0, 16) != 255) return 5;

    /* base == 0 auto-detect: 0x prefix -> hex */
    if (runtime.strtol("0xff", 0, 0) != 255) return 6;
    /* base == 0 auto-detect: leading 0 -> octal; 077 == 63 */
    if (runtime.strtol("077", 0, 0) != 63) return 7;
    /* base == 0 auto-detect: no prefix -> decimal */
    if (runtime.strtol("99", 0, 0) != 99) return 8;

    /* leading whitespace skipped */
    if (runtime.strtol("  42", 0, 10) != 42) return 9;

    /* glibc saturates instead of wrapping */
    runtime.errno = 0;
    v = runtime.strtol("9999999999999999999999", 0, 10);
    if (v != 9223372036854775807L) return 10;
    if (runtime.errno != 34) return 11;

    runtime.errno = 0;
    v = runtime.strtol("-9999999999999999999999", 0, 10);
    if (v != -9223372036854775807L - 1L) return 12;
    if (runtime.errno != 34) return 13;

    /* LONG_MIN is in range */
    runtime.errno = 0;
    v = runtime.strtol("-9223372036854775808", 0, 10);
    if (v != -9223372036854775807L - 1L) return 14;
    if (runtime.errno != 0) return 15;

    return 0;
}
