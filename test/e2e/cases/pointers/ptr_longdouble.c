// long double is 16 bytes, the widest scalar.  Pointer arithmetic over it must
// scale by 16 — a scale factor bug is most likely to show up here because 16
// does not divide evenly into a 4-byte stride that a lazy codegen might use.
// Also exercises the x87 10-byte load/store through a moving long double*.
// expect: 0
package main;
int main() {
    long double a[4];
    a[0] = 1.0L; a[1] = 2.0L; a[2] = 3.0L; a[3] = 4.0L;
    long double *p = a;
    if (*(p + 0) != 1.0L) return 1;
    if (*(p + 1) != 2.0L) return 2;
    if (*(p + 2) != 3.0L) return 3;
    if (*(p + 3) != 4.0L) return 4;
    /* index and pointer forms agree */
    if (p[2] != *(p + 2)) return 5;
    /* step back from a runtime base */
    long double *q = a + 3;
    if (*(q - 1) != 3.0L) return 6;
    if (*(q - 3) != 1.0L) return 7;
    /* element-count difference is 3, not 48 (the byte distance) */
    if (q - p != 3) return 8;
    return 0;
}
