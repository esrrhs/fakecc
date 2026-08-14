// qsort: sorts an array in ascending order using a caller-supplied
// comparator.  Pin an unsorted array, an array with duplicates, and a
// reverse-sorted array.  The comparator returns a-b for ints.
// expect: 0
package main;
extern void qsort(void *base, long n, long sz, int (*cmp)(const void *, const void *));

int cmp_int(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

int main() {
    int a[5];
    int i;

    /* unsorted */
    a[0] = 5; a[1] = 3; a[2] = 4; a[3] = 1; a[4] = 2;
    qsort(a, 5, 4, cmp_int);
    if (a[0] != 1 || a[1] != 2 || a[2] != 3 || a[3] != 4 || a[4] != 5) return 1;

    /* with duplicates */
    a[0] = 3; a[1] = 1; a[2] = 2; a[3] = 1; a[4] = 3;
    qsort(a, 5, 4, cmp_int);
    if (a[0] != 1 || a[1] != 1 || a[2] != 2 || a[3] != 3 || a[4] != 3) return 2;

    /* reverse sorted */
    a[0] = 5; a[1] = 4; a[2] = 3; a[3] = 2; a[4] = 1;
    qsort(a, 5, 4, cmp_int);
    if (a[0] != 1 || a[1] != 2 || a[2] != 3 || a[3] != 4 || a[4] != 5) return 3;

    /* single element: must not crash */
    a[0] = 42;
    qsort(a, 1, 4, cmp_int);
    if (a[0] != 42) return 4;

    return 0;
}
