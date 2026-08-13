// expect: 10
// Nested struct members via '.' access.
package main;
struct Inner { int x; int y; };
struct Outer { struct Inner in; int z; };
int main(void) {
    struct Outer o;
    o.in.x = 2;
    o.in.y = 3;
    o.z = 4;
    return o.in.x * o.in.y + o.z;
}
