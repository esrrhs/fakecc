// expect: 0
// A 16-byte vector passed and returned by value must round-trip both lanes.
package main;
typedef double V __attribute__((vector_size(16)));
V id(V v) { return v; }
int main(void) {
    V a = { 1.0, 2.0 };
    V b = id(a);
    if (b[0] != 1.0) return 1;
    if (b[1] != 2.0) return 2;
    return 0;
}
