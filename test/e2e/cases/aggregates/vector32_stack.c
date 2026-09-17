// expect: 0
// The 9th 32-byte vector does not fit in XMM0–XMM7 and must travel on
// a 32-byte-aligned stack slot as one YMM, not four 8-byte moves.
package main;
typedef int V __attribute__((vector_size(32)));
__attribute__((noinline)) V last(V a, V b, V c, V d, V e, V f, V g, V h, V i) {
    (void)a; (void)b; (void)c; (void)d;
    (void)e; (void)f; (void)g; (void)h;
    return i;
}
int main(void) {
    V z = { 0, 0, 0, 0, 0, 0, 0, 0 };
    V x = { 9, 10, 11, 12, 13, 14, 15, 16 };
    V r = last(z, z, z, z, z, z, z, z, x);
    if (r[0] != 9) return 1;
    if (r[7] != 16) return 2;
    return 0;
}
