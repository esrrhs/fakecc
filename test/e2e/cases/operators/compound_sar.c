// compound_sar: the >>= operator (arithmetic right shift).  Shift a
// positive value, shift by zero, and shift a negative value, which
// must sign-extend (fill with 1 bits).
// expect: 0
package main;
int main() {
    int x = 64;
    x >>= 2;
    if (x != 16) return 1;
    x = 7;
    x >>= 0;
    if (x != 7) return 2; /* shift by zero is a no-op */
    x = -16;
    x >>= 1;
    if (x != -8) return 3; /* sign-extends */
    return 0;
}
