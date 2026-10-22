/* Union members occupy the same storage. Little-endian byte lane updates. */
package main;
import runtime;

union Word {
    unsigned u;
    unsigned char b[4];
};

enum { N = 6000000 };

int main(void) {
    union Word w;
    unsigned i, acc;

    w.u = 0x12345678u;
    acc = 0;
    for (i = 0; i < N; i++) {
        w.b[i & 3u] = (unsigned char)(w.b[i & 3u] + (unsigned char)(i & 31u));
        w.b[(i + 1u) & 3u] ^= (unsigned char)(i * 13u);
        w.u = (w.u << 1) | (w.u >> 31);
        acc += w.u;
    }
    runtime.printf("pun %u\n", acc);
    return 0;
}
