// expect: 0
// Packed bitfields that spill into a later eightbyte are INTEGER+INTEGER.
// `{ unsigned long :60; unsigned :8 }` is 9 bytes; gcc passes rdi+esi, then
// the next int in edx — not a single rdi that drops the high byte.
package main;

struct __attribute__((packed)) P {
    unsigned long a : 60;
    unsigned b : 8;
};

struct __attribute__((packed)) Q {
    unsigned char a : 1;
    unsigned long long b : 64;
};

__attribute__((noinline))
int take_p(struct P p, int k) {
    if (p.b != 0xa5) return 2;
    return k;
}

__attribute__((noinline))
int take_q(struct Q q, int k) {
    if (q.a != 1) return 3;
    if (q.b != 0x8000000000000001ULL) return 4;
    return k;
}

int main(void) {
    struct P p;
    struct Q q;
    p.a = 0xfffffffffffffffUL;
    p.b = 0xa5;
    if (take_p(p, 7) != 7) return 1;
    q.a = 1;
    q.b = 0x8000000000000001ULL;
    if (take_q(q, 7) != 7) return 5;
    if (sizeof(struct P) != 9) return 6;
    if (sizeof(struct Q) != 9) return 7;
    return 0;
}
