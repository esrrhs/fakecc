// expect: 5
// Double stored in a struct member, then loaded back and used.  Verifies the
// fix covers 8-byte (double) member loads as well as 4-byte float loads.
package main;
struct D { double v; int k; };
int main(void) {
    struct D d;
    d.v = 1.5;
    d.k = 2;
    return (int)(d.v + d.v) + d.k;
}
