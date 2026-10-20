/* Short-circuit, ternary, and the comma operator, all with side effects. */
package main;
import runtime;

enum { N = 8000000 };

static unsigned bumps;

static int bump(int v) {
    bumps++;
    return v;
}

int main(void) {
    unsigned i, acc, a, b;

    bumps = 0;
    a = 1;
    b = 2;
    acc = 0;
    for (i = 0; i < N; i++) {
        unsigned bit = i & 1u;
        if ((bit && bump((int)(a & 3u))) || bump((int)(b & 1u)))
            acc += a;
        a = (bit ? a + b : (bump(1), b + 1u));
        b = (a < b) ? b + (i & 7u) : a ^ (i & 15u);
        acc ^= a + b;
    }
    runtime.printf("logic %u %u\n", acc, bumps);
    return 0;
}
