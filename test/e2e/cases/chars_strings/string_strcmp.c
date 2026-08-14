// str.strcmp: lexicographic compare of two C strings, returning <0/0/>0
// from the first differing byte interpreted as unsigned char.  The
// classic bug treats bytes >= 0x80 as negative and flips the sign.
// Pin equal/less/greater, the prefix rule, and the high-byte sign.
// expect: 0
package main;
import str;
int main() {
    unsigned char p[2], q[2];

    /* equal */
    if (str.strcmp("foo", "foo") != 0) return 1;
    /* a < b */
    if (str.strcmp("abc", "abd") >= 0) return 2;
    /* a > b */
    if (str.strcmp("abd", "abc") <= 0) return 3;
    /* prefix: shorter is less */
    if (str.strcmp("ab", "abc") >= 0) return 4;
    if (str.strcmp("abc", "ab") <= 0) return 5;
    /* high byte: 0xFF (255) must be > 0x7F (127) as unsigned char */
    p[0] = 0xFF; p[1] = 0;
    q[0] = 0x7F; q[1] = 0;
    if (str.strcmp((char *)p, (char *)q) <= 0) return 6;
    if (str.strcmp((char *)q, (char *)p) >= 0) return 7;
    return 0;
}
