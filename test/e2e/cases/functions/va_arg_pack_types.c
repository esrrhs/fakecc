// expect: 0
// Splice empty packs, promoted floats, doubles, pointers, __int128, and extra
// named wrapper parameters through __builtin_va_arg_pack.
package main;

int g;

__attribute__((noinline)) int
empty_ok(int n, ...) {
    return n + 1;
}

__attribute__((noinline)) double
dsum(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double a = va_arg(ap, double);
    double b = va_arg(ap, double);
    va_end(ap);
    return a + b;
}

__attribute__((noinline)) unsigned long long
ullmix(int n, ...) {
    va_list ap;
    va_start(ap, n);
    unsigned long long a = va_arg(ap, unsigned long long);
    int *p = va_arg(ap, int *);
    va_end(ap);
    return a + (unsigned long long)(*p);
}

__attribute__((noinline)) __int128
pick128(int n, ...) {
    va_list ap;
    va_start(ap, n);
    __int128 x = va_arg(ap, __int128);
    va_end(ap);
    return x;
}

__attribute__((noinline)) int
pair(int a, int b, ...) {
    va_list ap;
    va_start(ap, b);
    int c = va_arg(ap, int);
    int d = va_arg(ap, int);
    va_end(ap);
    return a + b + c + d;
}

__attribute__((noinline)) void
set_g(int n, ...) {
    va_list ap;
    va_start(ap, n);
    g = va_arg(ap, int);
    va_end(ap);
}

extern inline __attribute__((always_inline, gnu_inline)) int
wrap_empty(int n, ...) {
    return empty_ok(n, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) double
wrap_d(int n, ...) {
    return dsum(n, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) double
wrap_d_prefix(int n, ...) {
    return dsum(n, 1.0, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) unsigned long long
wrap_ull(int n, ...) {
    return ullmix(n, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) __int128
wrap128(int n, ...) {
    return pick128(n, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) int
wrap_pair(int a, int b, ...) {
    return pair(a, b + 1, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) void
wrap_void(int n, ...) {
    set_g(n, __builtin_va_arg_pack());
}

int main(void) {
    if (wrap_empty(4) != 5) return 1;
    if (wrap_d(2, 1.5, 2.25) != 3.75) return 2;
    if (wrap_d(2, 1.5f, 2.5) != 4.0) return 3;
    if (wrap_d_prefix(2, 2.5) != 3.5) return 4;

    int k = 20;
    if (wrap_ull(2, 100ULL, &k) != 120ULL) return 5;

    __int128 v = ((__int128)1 << 80) + 99;
    __int128 got = wrap128(1, v);
    if ((int)got != 99) return 6;
    if ((int)(got >> 80) != 1) return 7;

    if (wrap_pair(1, 2, 3, 4) != 11) return 8;

    wrap_void(1, 42);
    if (g != 42) return 9;
    return 0;
}
