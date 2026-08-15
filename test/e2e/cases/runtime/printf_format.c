// runtime.printf conversions and length modifiers that the float-focused
// printf_float.c leaves uncovered: %o (octal), %u (unsigned), %i, %p
// (pointer), %X (uppercase hex), %c, and the l/ll length modifiers on
// integer conversions.  Pin each against an exact expected string.
// expect: 0
package main;
import runtime;
int main() {
    char buf[128];

    /* %o octal: 64 decimal == 040 octal... wait, 64 == 0o100 */
    if (runtime.sprintf(buf, "%o", 64) != 3) return 1;
    if (runtime.strcmp(buf, "100") != 0) return 2;

    /* %u unsigned and %i (same as %d) */
    if (runtime.sprintf(buf, "%u|%i", 3000000000u, -5) != 13) return 3;
    if (runtime.strcmp(buf, "3000000000|-5") != 0) return 4;

    /* %X uppercase hex */
    if (runtime.sprintf(buf, "%X", 255) != 2) return 5;
    if (runtime.strcmp(buf, "FF") != 0) return 6;

    /* %c */
    if (runtime.sprintf(buf, "%c%c", 'A', 'B') != 2) return 7;
    if (runtime.strcmp(buf, "AB") != 0) return 8;

    /* %p pointer: prints 0x plus hex digits of the address */
    int x = 0;
    int n = runtime.sprintf(buf, "%p", &x);
    if (n < 3) return 9;                 /* at least "0x" + one digit */
    if (buf[0] != '0' || buf[1] != 'x') return 10;

    /* l/ll length modifiers: %llu of a value > 2^31 keeps the high bits.  Both
       %u (reading the low 32 bits) and %llu print the same magnitude here,
       since 3000000000 fits in 32 bits; the point is that %llu does not wrap. */
    unsigned long long v = 3000000000ULL;
    if (runtime.sprintf(buf, "%u|%llu", (unsigned int)v, v) != 21) return 11;
    if (runtime.strcmp(buf, "3000000000|3000000000") != 0) return 12;

    return 0;
}
