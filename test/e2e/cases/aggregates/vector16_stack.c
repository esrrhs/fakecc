// expect: 0
// The 9th 16-byte vector does not fit in XMM0–XMM7 and must travel on
// the stack as one 16-byte SSE+SSEUP slot, not two 8-byte moves.
package main;
typedef double V __attribute__((vector_size(16)));
__attribute__((noinline)) V last(V a, V b, V c, V d, V e, V f, V g, V h, V i) {
    (void)a; (void)b; (void)c; (void)d;
    (void)e; (void)f; (void)g; (void)h;
    return i;
}
int main(void) {
    V z = { 0.0, 0.0 };
    V x = { 3.0, 4.0 };
    V r = last(z, z, z, z, z, z, z, z, x);
    if (r[0] != 3.0) return 1;
    if (r[1] != 4.0) return 2;
    return 0;
}
