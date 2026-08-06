// expect: 9
// Struct by value as a return type: the callee fills a hidden destination slot
// (sret) and the caller reads members from it.  make(3,4).a + make(5,6).b.
package main;
struct S { int a; int b; };
struct S make(int x, int y) {
    struct S s;
    s.a = x; s.b = y;
    return s;
}
int main(void) {
    return make(3, 4).a + make(5, 6).b;
}
