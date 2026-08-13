// expect: 42
// Nested struct accessed through a pointer with '->'.
package main;
struct Inner { int x; int y; };
struct Outer { struct Inner in; int z; };
int main(void) {
    struct Outer o;
    struct Outer *p = &o;
    p->in.x = 40;
    p->in.y = 1;
    p->z = 1;
    return p->in.x + p->in.y + p->z;
}
