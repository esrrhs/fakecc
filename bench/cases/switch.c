/* Dense switch, sparse switch, and a GNU case range. */
package main;
import runtime;

enum { N = 4096 };
enum { ROUNDS = 1600 };

static unsigned char ops[N];
static unsigned state[64];

static unsigned dense(unsigned op, unsigned a, unsigned b) {
    switch (op & 15) {
    case 0: return a + b;
    case 1: return a - b;
    case 2: return a * (b | 1u);
    case 3: return a ^ b;
    case 4: return a & b;
    case 5: return a | b;
    case 6: return (a << (b & 7u)) | (a >> (8u - (b & 7u)));
    case 7: return a / (b | 1u);
    case 8 ... 11: return a + (op & 3u);
    default: return a ^ op;
    }
}

static unsigned sparse(unsigned op, unsigned a) {
    switch (op) {
    case 2: return a + 1u;
    case 40: return a ^ 0x5au;
    case 100: return a * 3u;
    case 700: return a - 9u;
    case 4000: return a | 7u;
    default: return a;
    }
}

int main(void) {
    int i, r;
    unsigned acc;

    for (i = 0; i < N; i++)
        ops[i] = (unsigned char)((i * 17 + 3) & 15);
    for (i = 0; i < 64; i++)
        state[i] = (unsigned)(i * 13 + 1);
    acc = 0;
    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < N; i++) {
            unsigned ix = (unsigned)(i & 63);
            state[ix] = dense(ops[i], state[ix], (unsigned)(i + r));
            if ((i & 31) == 0)
                state[ix] = sparse((unsigned)((i * 3) % 5000), state[ix]);
            acc += state[ix];
        }
    }
    runtime.printf("switch %u\n", acc);
    return 0;
}
