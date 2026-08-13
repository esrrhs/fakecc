// expect: 0
// Designated array initializer: `int a[6] = {[0] = 1, [2] = 10, 5, 6}`.
// Verifies (1) fields set by designator land at the right index regardless of
// order, (2) sparse slots are zero-filled, and (3) the positional tail resumes
// after the slot following the last designator.  Returns 0 on success, or a
// non-zero sentinel for the first failing sub-check.
package main;
int main() {
    int a[6] = {[0] = 1, [2] = 10, 5, 6};
    /* expected layout: a = {1, 0, 10, 5, 6, 0} */
    if (a[0] != 1) return 1;  /* [0] designator */
    if (a[1] != 0) return 2;  /* gap-filled zero */
    if (a[2] != 10) return 3; /* [2] designator */
    if (a[3] != 5) return 4;  /* positional tail resumes after designator */
    if (a[4] != 6) return 5;
    if (a[5] != 0) return 6;  /* unfilled tail */
    return 0;
}
