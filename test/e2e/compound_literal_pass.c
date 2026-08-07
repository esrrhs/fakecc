// expect: 0
// Compound literal passed by value to a function: `get_x((struct S){7, 9})`.
// Verifies the literal's bytes are copied into the callee's parameter and read
// back correctly.  Returns 0 on success or a failing sentinel.
package main;
struct S { int x; int y; };
int get_x(struct S s) {
    return s.x;
}
int main() {
    int r = get_x((struct S){7, 9});
    if (r != 7) return 1;
    return 0;
}
