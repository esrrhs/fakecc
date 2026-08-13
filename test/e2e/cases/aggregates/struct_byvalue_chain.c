// expect: 10
// Struct-by-value chain: return a struct, pass it straight to another
// by-value function.  Exercises sret on the callee side and the hidden-arg
// prepend on the caller side in one expression.
package main;
struct S { int a; int b; };
struct S make(int x, int y) {
    struct S s;
    s.a = x; s.b = y;
    return s;
}
int sum(struct S s) { return s.a + s.b; }
int main(void) {
    return sum(make(3, 7));
}
