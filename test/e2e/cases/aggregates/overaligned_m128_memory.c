// expect: 0
// SysV: padding eightbytes are NO_CLASS, not SSEUP, so
// `aligned(32) { __m128 }` is MEMORY (one stack blob), not a YMM.
package main;
typedef double V __attribute__((vector_size(16)));
struct __attribute__((aligned(32))) A { V x; };

__attribute__((noinline))
int take(struct A a, int k) {
    if ((int)a.x[0] != 7) return 1;
    if ((int)a.x[1] != 9) return 2;
    return k;
}

int main(void) {
    struct A a;
    a.x[0] = 7.0;
    a.x[1] = 9.0;
    if (sizeof(struct A) != 32) return 3;
    if (__alignof__(struct A) != 32) return 4;
    if (take(a, 0) != 0) return 5;
    return 0;
}
