/* Eratosthenes sieve. Integer-heavy, mostly byte stores and bit tests. */
package main;
import runtime;

enum { N = 8000000 };

static unsigned char bits[N / 8 + 1];

static int is_prime(int i) {
    return (bits[i >> 3] & (unsigned char)(1u << (i & 7))) != 0;
}

static void clear_prime(int i) {
    bits[i >> 3] = (unsigned char)(bits[i >> 3] & ~(1u << (i & 7)));
}

int main(void) {
    int i, p, count;
    unsigned long long sum;

    for (i = 0; i < N / 8 + 1; i++)
        bits[i] = 0xff;
    bits[0] = (unsigned char)(bits[0] & ~3u);

    for (p = 2; p * p < N; p++) {
        if (!is_prime(p))
            continue;
        for (i = p * p; i < N; i += p)
            clear_prime(i);
    }

    count = 0;
    sum = 0;
    for (i = 2; i < N; i++) {
        if (is_prime(i)) {
            count++;
            sum += (unsigned long long)i;
        }
    }
    runtime.printf("sieve %d %llu\n", count, sum);
    return 0;
}
