// Pointer arithmetic with an int array: p + n and p - n must scale by
// sizeof(int) (4), so p+1 lands on a[1], not one byte later.  The common
// codegen bug is to add the raw integer to the address, which lands in the
// middle of an element and reads garbage.
// expect: 0
package main;
int main() {
    int a[5];
    a[0] = 10; a[1] = 20; a[2] = 30; a[3] = 40; a[4] = 50;
    int *p = a;
    if (*(p + 0) != 10) return 1;
    if (*(p + 1) != 20) return 2;
    if (*(p + 2) != 30) return 3;
    if (*(p + 4) != 50) return 4;
    // Advance the pointer itself, then index back from the new base.
    p = p + 2;                 /* now points at a[2] */
    if (*p != 30) return 5;
    if (*(p - 1) != 20) return 6;
    if (*(p - 2) != 10) return 7;
    if (*(p + 1) != 40) return 8;
    return 0;
}
