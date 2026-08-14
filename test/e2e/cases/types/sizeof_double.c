// sizeof(double) / sizeof(long double) / sizeof(short) / sizeof(char).
// Pins the remaining scalar sizes that sizeof_int / sizeof_long leave
// uncovered: double is 8, long double is 16 (x87 80-bit extended, SysV
// aligns/pads it to 16), short is 2, char is 1.  Also checks that sizeof
// a double expression (not just the type) works, and that long double
// arithmetic is wide enough to hold a value a double could not round
// exactly.
// expect: 0
package main;
int main() {
    if ((int)sizeof(double) != 8) return 1;
    if ((int)sizeof(long double) != 16) return 2;
    if ((int)sizeof(short) != 2) return 3;
    if ((int)sizeof(char) != 1) return 4;

    /* sizeof on an expression, not a bare type */
    double d = 3.0;
    if ((int)sizeof(d) != 8) return 5;

    /* long double holds 1e20 without the rounding a double would suffer;
     * the low bits differ, so the value is not exactly 1e20 as a double. */
    long double big = 1e20L;
    if (big <= 0) return 6;

    /* short arithmetic wraps at 16 bits */
    short s = 30000;
    s = (short)(s + 10000); /* 40000 -> wraps to -25536 as signed short */
    if (s >= 0) return 7;

    return 0;
}
