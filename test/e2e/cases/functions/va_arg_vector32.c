// expect: 0
// SysV: unnamed __m256 / vector_size(32) to a variadic callee is MEMORY.
// Passing in ymm0 and reading overflow only works when the caller's local
// happens to sit at rbp+16 after the call — same-TU frames do not.
package main;
typedef int V __attribute__((vector_size(32)));

int pick(int n, ...) {
    va_list ap;
    va_start(ap, n);
    V v = va_arg(ap, V);
    va_end(ap);
    return v[0] + v[7];
}

int pick2(int n, ...) {
    va_list ap;
    va_start(ap, n);
    V a = va_arg(ap, V);
    V b = va_arg(ap, V);
    va_end(ap);
    return a[0] + b[7];
}

int named_then_var(V a, ...) {
    va_list ap;
    va_start(ap, a);
    V b = va_arg(ap, V);
    va_end(ap);
    return a[0] + b[7];
}

int main(void) {
    V x = {1, 2, 3, 4, 5, 6, 7, 8};
    V y = {10, 20, 30, 40, 50, 60, 70, 80};
    if (pick(0, x) != 9) return 1;
    if (pick2(0, x, y) != 81) return 2;
    if (named_then_var(x, y) != 81) return 3;
    return 0;
}
