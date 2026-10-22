/* 128-bit integer vector add and xor. */
package main;
import runtime;

typedef int v4si __attribute__((vector_size(16)));

enum { N = 4800000 };

int main(void) {
    v4si a, b, c;
    unsigned i, acc;

    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
    b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;
    acc = 0;
    for (i = 0; i < N; i++) {
        c = a + b;
        a = c ^ b;
        b[i & 3u] += (int)(i & 15u);
        acc += (unsigned)a[0] + (unsigned)a[1] + (unsigned)a[2] + (unsigned)a[3];
    }
    runtime.printf("vec %u\n", acc);
    return 0;
}
