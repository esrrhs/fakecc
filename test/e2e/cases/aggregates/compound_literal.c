// expect: 0
// Compound literal assigned to a struct variable: `struct S s = (struct S){1, 2}`.
// Verifies the bytes are copied by value (not aliased) and both fields read
// back correctly.  Returns 0 on success or a failing sentinel.
package main;
struct S { int x; int y; };
int main() {
    struct S s = (struct S){1, 2};
    if (s.x != 1) return 1;
    if (s.y != 2) return 2;
    return 0;
}
