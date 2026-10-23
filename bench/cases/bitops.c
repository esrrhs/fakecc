/* GCC bit builtins: clz, ctz, popcount, ffs, bswap in a hot loop. */
package main;
import runtime;

enum { N = 8000000 };

int main(void) {
    unsigned i, x, y, acc;

    x = 0x13579bd3u;
    y = 0x02468aceu;
    acc = 0;
    for (i = 0; i < N; i++) {
        /* Keep operands nonzero so clz/ctz are defined. */
        x = x * 1664525u + 1013904223u + (i | 1u);
        y = y * 22695477u + 1u + ((i + 3u) | 1u);
        if (x == 0u) x = 1u;
        if (y == 0u) y = 1u;
        acc += (unsigned)__builtin_clz(x);
        acc += (unsigned)__builtin_ctz(y);
        acc += (unsigned)__builtin_popcount(x ^ y);
        acc += (unsigned)__builtin_ffs(x);
        acc ^= __builtin_bswap32(x);
        acc ^= (unsigned)__builtin_popcountll(((unsigned long long)x << 32) | y);
        acc += (unsigned)__builtin_clzll(((unsigned long long)y << 17) | x);
    }
    runtime.printf("bitops %u\n", acc);
    return 0;
}
