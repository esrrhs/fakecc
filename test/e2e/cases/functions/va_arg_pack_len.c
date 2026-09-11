// expect: 0
// __builtin_va_arg_pack_len counts only anonymous extras at an always-inlined
// wrapper, and can decide whether to forward the pack at all.
package main;

__attribute__((noinline)) int
sink(int n, ...) {
    return n;
}

extern inline __attribute__((always_inline, gnu_inline)) int
count(int x, ...) {
    return x + __builtin_va_arg_pack_len();
}

extern inline __attribute__((always_inline, gnu_inline)) int
fwd_if_any(int x, ...) {
    if (__builtin_va_arg_pack_len() == 0)
        return x;
    return sink(x + __builtin_va_arg_pack_len(), __builtin_va_arg_pack());
}

int main(void) {
    if (count(10) != 10) return 1;
    if (count(10, 1) != 11) return 2;
    if (count(10, 1, 2, 3) != 13) return 3;
    if (fwd_if_any(7) != 7) return 4;
    if (fwd_if_any(7, 99) != 8) return 5;
    return 0;
}
