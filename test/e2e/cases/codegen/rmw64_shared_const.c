// expect: 0
// Shared narrow constant widened into a 64-bit frame RMW must stay live for
// later uses, and 64-bit global RMW must preserve the high 32 bits.
package main;

static unsigned long long g64 = 0xffffffffULL;

__attribute__((noinline))
static int keep_int(int x) {
    return x;
}

__attribute__((noinline))
static int run_add(unsigned long long *escape) {
    int step = 7;
    unsigned long long i = 100;
    *escape = (unsigned long long)&i;
    i = i + step;
    int b = keep_int(step);
    if (b != 7 || i != 107ULL) return 1;
    return 0;
}

__attribute__((noinline))
static int run_sub(unsigned long long *escape) {
    signed char delta = -3;
    long long j = 20;
    *escape = (unsigned long long)&j;
    j = j + delta;
    int c = keep_int(delta);
    if (c != -3 || j != 17LL) return 2;
    return 0;
}

int main(void) {
    unsigned long long sink = 0;
    g64 = g64 + 1;
    if (g64 != 0x100000000ULL) return 3;
    if (run_add(&sink) != 0) return 4;
    if (run_sub(&sink) != 0) return 5;
    return 0;
}
