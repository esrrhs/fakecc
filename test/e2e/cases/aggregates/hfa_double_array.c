// expect: 0
// A struct whose only member is double[2] is SSE+SSE (not INTEGER+INTEGER).
// Passing and returning it by value must round-trip both doubles.
package main;
struct H { double d[2]; };
struct H id(struct H s) { return s; }
int main(void) {
    struct H s;
    s.d[0] = 1.5;
    s.d[1] = 2.5;
    struct H t = id(s);
    if (t.d[0] != 1.5) return 1;
    if (t.d[1] != 2.5) return 2;
    return 0;
}
