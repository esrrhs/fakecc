/* __thread / TLS globals updated on the hot path. */
package main;
import runtime;

enum { N = 10000000 };

__thread unsigned tls_a;
__thread unsigned tls_b;
__thread unsigned tls_mix;

int main(void) {
    unsigned i, acc;

    tls_a = 1;
    tls_b = 7;
    tls_mix = 0;
    acc = 0;
    for (i = 0; i < N; i++) {
        tls_a = tls_a * 1664525u + 1013904223u + (i & 255u);
        tls_b ^= tls_a + (i << 1);
        tls_mix += tls_a ^ tls_b;
        /* Force an address-taken TLS path occasionally. */
        if ((i & 1023u) == 0u) {
            unsigned *pa = &tls_a;
            unsigned *pb = &tls_b;
            *pa ^= *pb;
        }
        acc += tls_mix;
    }
    runtime.printf("tls %u %u %u\n", acc, tls_a, tls_b);
    return 0;
}
