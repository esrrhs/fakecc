// expect: 0
// Nested always-inline wrappers each splice __builtin_va_arg_pack into the
// next call; pack_len is evaluated against the current wrapper's extras.
package main;

__attribute__((noinline)) int
add3(int n, ...) {
    va_list ap;
    va_start(ap, n);
    int a = va_arg(ap, int);
    int b = va_arg(ap, int);
    va_end(ap);
    return n + a + b;
}

extern inline __attribute__((always_inline, gnu_inline)) int
inner(int x, ...) {
    return add3(x, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) int
outer(int x, ...) {
    return inner(x + 1, __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) int
mid_len(int x, ...) {
    return add3(x + __builtin_va_arg_pack_len(), __builtin_va_arg_pack());
}

extern inline __attribute__((always_inline, gnu_inline)) int
outer_len(int x, ...) {
    return mid_len(x, __builtin_va_arg_pack());
}

int main(void) {
    if (outer(1, 2, 3) != 7) return 1;
    if (outer_len(10, 1, 2) != 15) return 2;
    return 0;
}
