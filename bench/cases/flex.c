/* Flexible array member: growable payload via a static storage blob. */
package main;
import runtime;

enum { CAP = 16384 };
enum { ROUNDS = 4000 };

struct Hdr {
    int n;
    unsigned d[];
};

static unsigned char blob[sizeof(struct Hdr) + CAP * sizeof(unsigned)];

int main(void) {
    struct Hdr *h;
    int i, r, n;
    unsigned acc;

    h = (struct Hdr *)(void *)blob;
    n = 256;
    h->n = n;
    for (i = 0; i < n; i++)
        h->d[i] = (unsigned)(i * 17 + 3);
    acc = 0;
    for (r = 0; r < ROUNDS; r++) {
        n = 256 + (r % 11) * 256;
        if (n > CAP)
            n = CAP;
        h->n = n;
        for (i = 0; i < n; i++)
            h->d[i] = h->d[i % 256] + (unsigned)r + (unsigned)i;
        for (i = 1; i < n - 1; i++)
            h->d[i] = (h->d[i - 1] * 3u + h->d[i] + h->d[i + 1]) & 0xffffu;
        for (i = 0; i < n; i += 16)
            acc += h->d[i];
        acc ^= (unsigned)h->n;
    }
    runtime.printf("flex %u %d\n", acc, h->n);
    return 0;
}
