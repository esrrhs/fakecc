// expect: 55
// `long long` arithmetic — long long is 64-bit (width 8), same as long.
// Returns d (a long-long value narrowed to int) to prove the 64-bit result.
// Verifies `long long`, `long long int`, and `unsigned long long` parse and
// that 64-bit operations produce the correct result.
package main;
int main() {
    long long a = 100;
    long long int b = 200;
    long long c = a + b - 245;          /* 55 */
    unsigned long long u = 1ull;
    u = u + 2ull;                        /* 3 */
    if (c != 55) return 1;
    if (u != 3ull) return 2;
    /* narrowing coercion on assignment from long long to int */
    int d = c;                           /* 55 */
    return d;
}
