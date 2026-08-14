// memcpy: copies n bytes from src to dst and returns dst.
// A codegen bug that mis-scales the loop or passes the wrong size
// leaves the destination short or with garbage.  We copy a string
// (including its terminator), verify every byte, and check the
// return value is dst.
// expect: 0
package main;
extern void *memcpy(void *dst, const void *src, unsigned long n);
int main() {
    char src[6];
    src[0] = 'H'; src[1] = 'e'; src[2] = 'l'; src[3] = 'l'; src[4] = 'o'; src[5] = 0;
    char dst[6];
    /* poison dst so we know memcpy actually wrote each byte */
    dst[0] = 'X'; dst[1] = 'X'; dst[2] = 'X'; dst[3] = 'X'; dst[4] = 'X'; dst[5] = 'X';
    char *r = (char *)memcpy(dst, src, 6);
    if (r != dst) return 1;
    if (dst[0] != 'H') return 2;
    if (dst[1] != 'e') return 3;
    if (dst[2] != 'l') return 4;
    if (dst[3] != 'l') return 5;
    if (dst[4] != 'o') return 6;
    if (dst[5] != 0) return 7;
    return 0;
}
