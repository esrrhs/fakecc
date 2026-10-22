/* Computed goto used as the dispatch of a tiny interpreter. */
package main;
import runtime;

enum { N = 30000000 };

int main(void) {
    void *ops[] = { &&op_add, &&op_xor, &&op_sub, &&op_rot,
                    &&op_or, &&op_and, &&op_mix, &&op_inc };
    unsigned pc = 0;
    unsigned a = 1;
    unsigned b = 7;
    unsigned acc = 0;

    goto *ops[0];
op_add:
    a += b;
    goto next;
op_xor:
    a ^= b;
    goto next;
op_sub:
    a -= b | 1u;
    goto next;
op_rot:
    a = (a << 3) | (a >> 29);
    goto next;
op_or:
    a |= b;
    goto next;
op_and:
    a &= b | 0xffu;
    goto next;
op_mix:
    b = (b << 1) | (a & 1u);
    goto next;
op_inc:
    b += 3u;
    goto next;
next:
    acc += a;
    pc++;
    if (pc < N)
        goto *ops[pc & 7u];
    runtime.printf("cgoto %u\n", acc);
    return 0;
}
