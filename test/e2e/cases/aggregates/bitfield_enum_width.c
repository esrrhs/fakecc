// expect: 5
// Bitfield width may be a constant expression (here an enum constant).
package main;
enum { N = 3 };
struct S { unsigned x : N; };
int main(void) {
    struct S s;
    s.x = 5;
    return (int)s.x;
}
