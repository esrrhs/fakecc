// expect: 0
// Restoring 128-bit division must keep a 129-bit remainder when the high bit
// of the divisor is set (2^127).
package main;
int main(void) {
    unsigned __int128 d = (unsigned __int128)1 << 127;
    unsigned __int128 n = (unsigned __int128)-1;
    unsigned __int128 q = n / d;
    unsigned __int128 r = n % d;
    if (q != (unsigned __int128)1) return 1;
    if (r != (d - 1)) return 2;
    unsigned __int128 n2 = d + 5;
    if (n2 / d != 1) return 3;
    if (n2 % d != 5) return 4;
    return 0;
}
