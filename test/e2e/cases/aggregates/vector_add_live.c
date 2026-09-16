// expect: 0
// Vector ALU must not clobber allocatable GP/XMM homes of values live
// across the operation (RDI / XMM0 were previously hard-coded scratches).
package main;
typedef int V __attribute__((vector_size(16)));
__attribute__((noinline)) int after(int n, V r) {
    if (r[0] != 6) return 10;
    if (r[1] != 8) return 11;
    if (r[2] != 10) return 12;
    if (r[3] != 12) return 13;
    return n;
}
int main(void) {
    int n = 7;
    V a = { 1, 2, 3, 4 };
    V b = { 5, 6, 7, 8 };
    V r = a + b;
    n = n + 1;
    if (after(n, r) != 8) return 1;
    return 0;
}
