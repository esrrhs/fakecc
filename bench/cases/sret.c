/* Ten scalar parameters (past the register file) and a returned struct. */
package main;
import runtime;

struct Quad {
    int a, b, c, d;
};

static int sum10(int a, int b, int c, int d, int e,
                 int f, int g, int h, int i, int j) {
    return a + b + c + d + e + f + g + h + i + j;
}

static struct Quad pack(int a, int b, int c, int d) {
    struct Quad q;
    q.a = a;
    q.b = b;
    q.c = c;
    q.d = d;
    return q;
}

enum { N = 3200000 };

int main(void) {
    int i;
    unsigned acc;
    struct Quad q;

    q.a = 1;
    q.b = 2;
    q.c = 3;
    q.d = 4;
    acc = 0;
    for (i = 0; i < N; i++) {
        int s = sum10(q.a, q.b, q.c, q.d, i, i + 1, i + 2, i + 3, i + 4, i + 5);
        q = pack(s, q.a, q.b, q.c + (i & 7));
        acc += (unsigned)q.a ^ (unsigned)q.d;
    }
    runtime.printf("sret %u\n", acc);
    return 0;
}
