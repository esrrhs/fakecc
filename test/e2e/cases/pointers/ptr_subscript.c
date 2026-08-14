// Subscript and pointer-arithmetic equivalence: a[i] == *(a+i) == *(i+a) == i[a].
// The compiler lowers all of these the same way, but it is worth pinning down —
// a regression that special-cased the a[i] form would leave i[a] reading the
// wrong element.  Also exercises a pointer that is itself an array element
// (q = &a[2]), so the base address is not a compile-time constant.
// expect: 0
package main;
int main() {
    int a[5];
    a[0] = 11; a[1] = 22; a[2] = 33; a[3] = 44; a[4] = 55;
    /* a[i] vs *(a+i) vs *(i+a) vs i[a] */
    if (a[0] != *(a + 0)) return 1;
    if (a[1] != *(a + 1)) return 2;
    if (a[2] != *(2 + a)) return 3;   /* commutative */
    if (a[3] != 3[a])     return 4;   /* i[a] form */
    if (a[4] != *(a + 4)) return 5;
    /* base is a runtime pointer, not the array name */
    int *q = &a[2];
    if (q[0] != 33) return 6;    /* q[0] == a[2] */
    if (q[1] != 44) return 7;    /* q[1] == a[3] */
    if (q[-1] != 22) return 8;   /* q[-1] == a[1] */
    if (q[-2] != 11) return 9;   /* q[-2] == a[0] */
    if (*(q + 1) != q[1]) return 10;
    return 0;
}
