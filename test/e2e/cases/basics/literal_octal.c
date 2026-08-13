// expect: 0
// Octal integer literals: a leading `0` followed by octal digits.
// Verifies `077` (63), `010` (8), plain `0`, arithmetic, and that an octal
// value can be used as an array dimension.  Returns 0 on success, or a
// non-zero sentinel for the first failing sub-check.
package main;
int main() {
    int a = 077;   /* 63 */
    int b = 010;   /* 8 */
    int c = 0;     /* 0 */
    int d = 0100;  /* 64 */

    if (a != 63) return 1;
    if (b != 8) return 2;
    if (c != 0) return 3;
    if (d != 64) return 4;

    /* decimal literals must still work and not be misread as octal */
    int dec = 123;
    if (dec != 123) return 5;

    /* octal as an array dimension */
    int arr[010];  /* 8 elements */
    arr[7] = 77;
    if (arr[7] != 77) return 6;

    return 0;
}
