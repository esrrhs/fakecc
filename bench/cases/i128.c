/* 128-bit multiply-add. The printed value is the low and high halves. */
package main;
import runtime;

enum { N = 4000000 };

int main(void) {
    unsigned __int128 x;
    unsigned i;
    unsigned long long lo, hi;

    x = 1;
    for (i = 0; i < N; i++)
        x = x * 0x9e3779b97f4a7c15ULL + (unsigned __int128)i + 1u;
    lo = (unsigned long long)x;
    hi = (unsigned long long)(x >> 64);
    runtime.printf("i128 %llu %llu\n", lo, hi);
    return 0;
}
