/* ChaCha20-style ARX rounds over a buffer. Integer rotate/xor/add. */
package main;
import runtime;

enum { WORDS = 16384 };
enum { ROUNDS = 1400 };

static unsigned int st[WORDS];

static unsigned int rotl(unsigned int x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void quarter(unsigned int *a, unsigned int *b,
                    unsigned int *c, unsigned int *d) {
    *a += *b; *d = rotl(*d ^ *a, 16);
    *c += *d; *b = rotl(*b ^ *c, 12);
    *a += *b; *d = rotl(*d ^ *a, 8);
    *c += *d; *b = rotl(*b ^ *c, 7);
}

int main(void) {
    int i, r;
    unsigned int acc;

    for (i = 0; i < WORDS; i++)
        st[i] = (unsigned int)(i * 2654435761u + 0x9e3779b9u);

    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < WORDS; i += 4)
            quarter(&st[i], &st[i + 1], &st[i + 2], &st[i + 3]);
        for (i = 0; i < WORDS; i += 16) {
            quarter(&st[i], &st[i + 4], &st[i + 8], &st[i + 12]);
            quarter(&st[i + 1], &st[i + 5], &st[i + 9], &st[i + 13]);
            quarter(&st[i + 2], &st[i + 6], &st[i + 10], &st[i + 14]);
            quarter(&st[i + 3], &st[i + 7], &st[i + 11], &st[i + 15]);
        }
    }

    acc = 0;
    for (i = 0; i < WORDS; i++)
        acc ^= st[i];
    runtime.printf("chacha %u\n", acc);
    return 0;
}
