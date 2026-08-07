// expect: 0
// Hexadecimal integer literals: `0x` / `0X` prefix with hex digits.
// Verifies basic hex, lowercase, the `u` suffix, arithmetic, and that a hex
// value can be used as an array dimension.  Returns 0 on success, or a
// non-zero sentinel for the first failing sub-check.
package main;
int main() {
    int a = 0xFF;            /* 255 */
    int b = 0x10;            /* 16 */
    int c = 0x0;             /* 0 */
    unsigned int u = 0XFFu;  /* 255, suffix + uppercase X */
    int sum = 0xFF + 1;      /* 256 */

    if (a != 255) return 1;
    if (b != 16) return 2;
    if (c != 0) return 3;
    if (u != 255) return 4;
    if (sum != 256) return 5;

    /* hex as an array dimension */
    int arr[0x4];            /* 4 elements */
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40;
    if (arr[0] + arr[3] != 50) return 6;

    return 0;
}
