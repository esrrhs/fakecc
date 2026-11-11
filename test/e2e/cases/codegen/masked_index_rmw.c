// expect: 0
/* Masked local-array RMW (a[i&3] += ...) must keep C semantics when the
 * i&3 SSA temp is rematerialized at each SIB use instead of spilled.
 * Also covers a named copy of the mask and a shared index/value use. */
package main;

int main(void) {
    int a[4];
    int i, k, s;

    for (i = 0; i < 4; i++)
        a[i] = i + 1;
    for (i = 0; i < 16; i++)
        a[i & 3] += (i & 7);
    /* 16 iterations: i&3 cycles 0,1,2,3; i&7 cycles 0..7 twice.
     * Per slot j in 0..3, added values are j and j+4, twice: 2*(j + j+4) = 4*j + 8.
     * a[j] starts at j+1, ends at 5*j + 9.  Sum = 9+14+19+24 = 66. */
    s = 0;
    for (i = 0; i < 4; i++)
        s += a[i];
    if (s != 66)
        return 1;

    for (i = 0; i < 4; i++)
        a[i] = 0;
    s = 0;
    for (i = 0; i < 8; i++) {
        k = i & 3;
        a[k] += k;
        a[i & 3] += (i & 3);
        s += k;
    }
    /* k and (i&3) each add 0,1,2,3 twice → each slot j gets 2*j + 2*j = 4*j.
     * s = (0+1+2+3)*2 = 12.  Sum of a = 0+4+8+12 = 24.  Total 36. */
    if (s + a[0] + a[1] + a[2] + a[3] != 36)
        return 2;

    return 0;
}
