// expect: 0
// 10-arg call plus a 16-byte struct return, in a loop (sret bench shape).
package main;

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

int main(void) {
    int i;
    unsigned acc;
    struct Quad q;
    q.a = 1;
    q.b = 2;
    q.c = 3;
    q.d = 4;
    acc = 0;
    for (i = 0; i < 20; i = i + 1) {
        int s;
        s = sum10(q.a, q.b, q.c, q.d, i, i + 1, i + 2, i + 3, i + 4, i + 5);
        q = pack(s, q.a, q.b, q.c + (i & 7));
        acc = acc + ((unsigned)q.a ^ (unsigned)q.d);
    }
    if (acc != 19294658u) return 1;
    return 0;
}
