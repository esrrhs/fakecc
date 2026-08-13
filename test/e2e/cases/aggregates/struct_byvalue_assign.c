// expect: 7
// Struct by value assignment: `r = make(3,4)` must copy bytes, not the pointer.
package main;
struct S { int a; int b; };
struct S make(int x, int y) {
    struct S s;
    s.a = x; s.b = y;
    return s;
}
int main(void) {
    struct S r;
    r = make(3, 4);
    return r.a + r.b;
}
