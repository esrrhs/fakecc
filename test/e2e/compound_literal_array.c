// expect: 0
// Array compound literal assigned to a pointer: `int *p = (int[]){1, 2, 3}`.
// Verifies the array length is inferred (3) and elements read back through
// the pointer.  Returns 0 on success or a failing sentinel.
package main;
int main() {
    int *p = (int[]){1, 2, 3};
    if (p[0] != 1) return 1;
    if (p[1] != 2) return 2;
    if (p[2] != 3) return 3;
    return 0;
}
