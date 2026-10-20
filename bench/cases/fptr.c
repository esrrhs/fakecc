/* Indirect calls through a function-pointer table. */
package main;
import runtime;

typedef unsigned (*binop)(unsigned, unsigned);

static unsigned op_add(unsigned a, unsigned b) { return a + b; }
static unsigned op_sub(unsigned a, unsigned b) { return a - b; }
static unsigned op_xor(unsigned a, unsigned b) { return a ^ b; }
static unsigned op_mul(unsigned a, unsigned b) { return a * (b | 1u); }
static unsigned op_and(unsigned a, unsigned b) { return a & b; }
static unsigned op_or(unsigned a, unsigned b) { return a | b; }
static unsigned op_mix(unsigned a, unsigned b) { return (a << (b & 3u)) ^ b; }
static unsigned op_div(unsigned a, unsigned b) { return a / (b | 1u); }

static binop table[8] = {
    op_add, op_sub, op_xor, op_mul, op_and, op_or, op_mix, op_div
};

enum { N = 8000000 };

int main(void) {
    unsigned a, b, i, acc;
    a = 1;
    b = 3;
    acc = 0;
    for (i = 0; i < N; i++) {
        binop fn = table[i & 7u];
        a = fn(a, b + (i & 15u));
        b = table[(i + 3u) & 7u](b, a | 1u);
        acc += a ^ b;
    }
    runtime.printf("fptr %u\n", acc);
    return 0;
}
