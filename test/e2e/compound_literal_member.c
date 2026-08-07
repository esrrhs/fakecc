// expect: 0
// Member access on a compound literal: `(struct S){1, 2}.x`.  Verifies the
// literal is materialized as an lvalue and the field is read back.  Returns 0
// on success or a failing sentinel.
package main;
struct S { int x; int y; };
int main() {
    if ((struct S){1, 2}.x != 1) return 1;
    if ((struct S){1, 2}.y != 2) return 2;
    return 0;
}
