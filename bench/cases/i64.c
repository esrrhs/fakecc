/* 64-bit multiply, divide, and remainder. */
package main;
import runtime;

enum { N = 8000000 };

int main(void) {
    unsigned long long a, b, acc;
    unsigned i;

    a = 0x123456789abcdefULL;
    b = 0xfedcba987654321ULL;
    acc = 0;
    for (i = 0; i < N; i++) {
        unsigned long long d = (b | 1ull) + (unsigned long long)(i & 1023u);
        a = a * 0x9e3779b97f4a7c15ULL + (unsigned long long)i;
        b = b ^ (a >> 17);
        acc += a / d;
        acc ^= a % d;
    }
    runtime.printf("i64 %llu\n", acc);
    return 0;
}
