// expect: 0
// Default: vector_size(64) is MEMORY (64-byte stack).  -mavx512f uses one ZMM.
package main;
typedef int V __attribute__((vector_size(64)));
__attribute__((noinline)) V id(V v) { return v; }
int main(void) {
    V a = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    V b = id(a);
    if (b[0] != 1) return 1;
    if (b[7] != 8) return 2;
    if (b[15] != 16) return 3;
    return 0;
}
