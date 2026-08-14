// std.strtol: converts a string to a long in a given base, reporting where
// parsing stopped through an endptr.  Pin base-10, sign, endptr, hex,
// and the base==0 auto-detect path (hex via 0x, octal via 0, else
// decimal).
// expect: 0
package main;
import std;
int main() {
    char *end;

    if (std.strtol("123", 0, 10) != 123) return 1;
    if (std.strtol("-42", 0, 10) != -42) return 2;

    /* endptr points at the first non-digit character */
    long v = std.strtol("12abc", &end, 10);
    if (v != 12) return 3;
    if (end[0] != 'a') return 4;

    /* explicit hex */
    if (std.strtol("0xff", 0, 16) != 255) return 5;

    /* base == 0 auto-detect: 0x prefix -> hex */
    if (std.strtol("0xff", 0, 0) != 255) return 6;
    /* base == 0 auto-detect: leading 0 -> octal; 077 == 63 */
    if (std.strtol("077", 0, 0) != 63) return 7;
    /* base == 0 auto-detect: no prefix -> decimal */
    if (std.strtol("99", 0, 0) != 99) return 8;

    /* leading whitespace skipped */
    if (std.strtol("  42", 0, 10) != 42) return 9;

    return 0;
}
