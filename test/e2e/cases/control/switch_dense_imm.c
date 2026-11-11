// expect: 0
// Dense switch on (op & 15) plus immediate bitwise in the arms
// (shared epilogue / and $imm / or $imm).
package main;

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

int main(void) {
    unsigned acc;
    int i;
    acc = 0;
    for (i = 0; i < 256; i++)
        acc += dense((unsigned)i, (unsigned)(i * 3 + 1), (unsigned)(i + 7));
    return acc == 1285775u ? 0 : 1;
}
