// expect: 0
// `restrict` qualifier: parse a restrict-qualified pointer (fakecc treats
// restrict as a no-op hint without an optimizer).  Verifies it type-checks and
// the variable reads back correctly.  Returns 0 on success or a failing
// sentinel.
package main;
int main() {
    int val = 7;
    int * restrict x = &val;
    if (*x != 7) return 1;
    return 0;
}
