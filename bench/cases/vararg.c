/* Variadic walk: va_start / va_arg / va_end on every call. */
package main;
import runtime;

static unsigned mix(int n, ...) {
    va_list ap;
    int i;
    unsigned s;
    va_start(ap, n);
    s = 2166136261u;
    for (i = 0; i < n; i++) {
        unsigned v = va_arg(ap, unsigned);
        s ^= v;
        s *= 16777619u;
    }
    va_end(ap);
    return s;
}

enum { N = 1600000 };

int main(void) {
    unsigned i, acc, a, b, c, d, e, f;
    acc = 0;
    a = 1; b = 2; c = 3; d = 4; e = 5; f = 6;
    for (i = 0; i < N; i++) {
        a = mix(6, a, b, c, d, e, f);
        b += i & 31u;
        c ^= a;
        d += b;
        e = mix(3, e, a, i);
        f += d & 255u;
        acc += a ^ e;
    }
    runtime.printf("vararg %u\n", acc);
    return 0;
}
