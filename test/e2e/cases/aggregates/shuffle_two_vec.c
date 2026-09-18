// expect: 0
// 3-arg __builtin_shuffle reads v2 when a mask index is >= lane count.
package main;
typedef int V __attribute__((vector_size(16)));
int main(void) {
    V a = {1, 2, 3, 4};
    V b = {5, 6, 7, 8};
    V m = {0, 5, 2, 7};
    V r = __builtin_shuffle(a, b, m);
    if (r[0] != 1) return 1;
    if (r[1] != 6) return 2;
    if (r[2] != 3) return 3;
    if (r[3] != 8) return 4;
    return 0;
}
