// expect: 0
// An 8-byte integer vector is SSE-class (XMM), same as an 8-byte float
// vector.  Passing and returning it by value must round-trip both lanes.
package main;
typedef int V __attribute__((vector_size(8)));
__attribute__((noinline)) V id(V v) { return v; }
int main(void) {
    V a = { 3, 4 };
    V b = id(a);
    if (b[0] != 3) return 1;
    if (b[1] != 4) return 2;
    return 0;
}
