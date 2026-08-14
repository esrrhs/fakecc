// memcmp: lexicographic compare of the first n bytes, returning
// <0 / 0 / >0 from the first differing byte interpreted as unsigned
// char.  A common bug compares as signed char, which flips the sign
// for any byte >= 0x80.  Pin the sign for high bytes and the n==0
// edge case.
// expect: 0
package main;
extern int memcmp(const void *a, const void *b, unsigned long n);
int main() {
    char x[4], y[4], z[4];
    unsigned char p[1], q[1];

    x[0] = 'a'; x[1] = 'b'; x[2] = 'c'; x[3] = 0;
    y[0] = 'a'; y[1] = 'b'; y[2] = 'd'; y[3] = 0;
    z[0] = 'a'; z[1] = 'b'; z[2] = 'c'; z[3] = 0;

    /* a < b */
    if (memcmp(x, y, 3) >= 0) return 1;
    /* a > b */
    if (memcmp(y, x, 3) <= 0) return 2;
    /* equal */
    if (memcmp(x, z, 3) != 0) return 3;
    /* n == 0 is always equal, even if pointers differ */
    if (memcmp(x, y, 0) != 0) return 4;
    /* high bytes: 0xFF (255) must be > 0x01 as unsigned char */
    p[0] = 0xFF; q[0] = 0x01;
    if (memcmp(p, q, 1) <= 0) return 5;
    if (memcmp(q, p, 1) >= 0) return 6;
    return 0;
}
