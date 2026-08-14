// str.memmove: like str.memcpy but handles overlapping regions.  The classic
// bug is copying forward when dst > src, which overwrites bytes of
// src before they are read.  Test both directions of overlap plus
// the non-overlapping case, and verify the return value.
// expect: 0
package main;
import str;
int main() {
    char a[5];
    char b[5];
    char *r;

    /* non-overlapping */
    a[0] = 'a'; a[1] = 'b'; a[2] = 'c'; a[3] = 'd'; a[4] = 0;
    r = (char *)str.memmove(b, a, 5);
    if (r != b) return 1;
    if (b[0] != 'a' || b[1] != 'b' || b[2] != 'c' || b[3] != 'd') return 2;

    /* overlapping: dst > src, must copy backward.
       buf = "12345", move it +1 -> expect "112345". */
    char buf[6];
    buf[0] = '1'; buf[1] = '2'; buf[2] = '3'; buf[3] = '4'; buf[4] = '5'; buf[5] = 0;
    r = (char *)str.memmove(buf + 1, buf, 5);
    if (r != buf + 1) return 3;
    if (buf[0] != '1') return 4;
    if (buf[1] != '1') return 5;   /* original buf[0] */
    if (buf[2] != '2') return 6;
    if (buf[3] != '3') return 7;
    if (buf[4] != '4') return 8;
    if (buf[5] != '5') return 9;   /* original buf[4], now at buf[5] */

    /* overlapping: dst < src, must copy forward.
       c = "abcde", move it -1 -> expect "bcde\0". */
    char c[6];
    c[0] = 'a'; c[1] = 'b'; c[2] = 'c'; c[3] = 'd'; c[4] = 'e'; c[5] = 0;
    r = (char *)str.memmove(c, c + 1, 5);
    if (r != c) return 10;
    if (c[0] != 'b') return 11;
    if (c[1] != 'c') return 12;
    if (c[2] != 'd') return 13;
    if (c[3] != 'e') return 14;
    if (c[4] != 0) return 15;

    return 0;
}
