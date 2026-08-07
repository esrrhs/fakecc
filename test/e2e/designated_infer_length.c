// expect: 0
// Designated initializer inferring array length: `int a[] = {[3] = 1}` should
// infer length 4 (max designator index + 1).  Verifies the inferred length and
// zero-fill of the leading gap.  Returns 0 on success or a failing sentinel.
package main;
int main() {
    int a[] = {[3] = 1};
    if (a[0] != 0) return 1; /* gap-filled zero */
    if (a[1] != 0) return 2;
    if (a[2] != 0) return 3;
    if (a[3] != 1) return 4; /* [3] designator */
    return 0;
}
