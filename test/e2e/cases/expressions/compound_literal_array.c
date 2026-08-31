// expect: 0
package main;

/* Array-typed file-scope compound literal: `&(int[]){...}`.  The literal has
 * static storage; decay yields the address of element 0, and indexed element
 * addresses must match the layout. */
static int *p0 = &(int[]){10, 20, 30}[0];
static int *p2 = &(int[]){10, 20, 30}[2];
static int *base = &(int[]){10, 20, 30}[0];

int main(void) {
    if (*p0 != 10) return 1;
    if (*p2 != 30) return 2;
    /* base[1] must be contiguous after base[0]. */
    if (base[1] != 20) return 3;
    return 0;
}
