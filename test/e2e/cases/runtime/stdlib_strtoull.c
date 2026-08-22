// runtime.strtoull: converts a string to an unsigned long long in a given
// base, reporting where parsing stopped through an endptr.  Pin base-10,
// endptr, explicit hex, the base==0 auto-detect path, and glibc saturation
// to ULLONG_MAX on overflow.
// expect: 0
package main;
import runtime;
int main() {
    char *end;

    if (runtime.strtoull("123", 0, 10) != 123ULL) return 1;

    /* endptr points at the first non-digit character */
    unsigned long long v = runtime.strtoull("12abc", &end, 10);
    if (v != 12ULL) return 2;
    if (end[0] != 'a') return 3;

    /* explicit hex, uppercase */
    if (runtime.strtoull("FF", 0, 16) != 255ULL) return 4;

    /* base == 0 auto-detect: 0x prefix -> hex */
    if (runtime.strtoull("0xff", 0, 0) != 255ULL) return 5;
    /* base == 0 auto-detect: leading 0 -> octal; 077 == 63 */
    if (runtime.strtoull("077", 0, 0) != 63ULL) return 6;
    /* base == 0 auto-detect: no prefix -> decimal */
    if (runtime.strtoull("99", 0, 0) != 99ULL) return 7;

    /* leading whitespace skipped */
    if (runtime.strtoull("  42", 0, 10) != 42ULL) return 8;

    /* a value that overflows 32-bit but fits 64-bit: 3000000000 */
    if (runtime.strtoull("3000000000", 0, 10) != 3000000000ULL) return 9;

    runtime.errno = 0;
    v = runtime.strtoull("18446744073709551616", 0, 10);
    if (v != 18446744073709551615ULL) return 10;
    if (runtime.errno != 34) return 11;

    return 0;
}
