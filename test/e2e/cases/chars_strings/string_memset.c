// memset: fills n bytes of dst with byte c and returns dst.  Used
// everywhere; a wrong byte mask or wrong count leaves gaps.  Test a
// high byte (0xAB) and the common zero-fill, and check the return
// value.
// expect: 0
package main;
extern void *memset(void *dst, int c, unsigned long n);
int main() {
    char buf[8];
    int i;
    char *r;

    r = (char *)memset(buf, 0xAB, 8);
    if (r != buf) return 1;
    i = 0;
    while (i < 8) {
        if ((unsigned char)buf[i] != 0xAB) return 2;
        i = i + 1;
    }

    /* zero fill, the common case */
    r = (char *)memset(buf, 0, 8);
    if (r != buf) return 3;
    i = 0;
    while (i < 8) {
        if (buf[i] != 0) return 4;
        i = i + 1;
    }
    return 0;
}
