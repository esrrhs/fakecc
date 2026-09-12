// expect: 0
// Nested {double; long} keeps SSE+INTEGER eightbytes rather than collapsing
// the inner struct to a single INTEGER class.
package main;
struct Inner { double d; long l; };
struct Outer { struct Inner in; };
struct Outer id(struct Outer s) { return s; }
int main(void) {
    struct Outer s;
    s.in.d = 2.5;
    s.in.l = 7;
    struct Outer t = id(s);
    if (t.in.d != 2.5) return 1;
    if (t.in.l != 7) return 2;
    return 0;
}
