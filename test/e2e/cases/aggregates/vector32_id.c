// expect: 0
// A 32-byte vector is one YMM (SSE+SSEUP×3): pass and return in ymm0.
package main;
typedef int V __attribute__((vector_size(32)));
__attribute__((noinline)) V id(V v) { return v; }
int main(void) {
    V a = { 1, 2, 3, 4, 5, 6, 7, 8 };
    V b = id(a);
    if (b[0] != 1) return 1;
    if (b[3] != 4) return 2;
    if (b[7] != 8) return 3;
    return 0;
}
