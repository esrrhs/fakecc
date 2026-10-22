/* Designated initializers, range initializers, and compound literals. */
package main;
import runtime;

struct Rec {
    int a, b, c, d;
};

enum { N = 6000000 };

static int seed_row[8] = { [0 ... 3] = 1, [4] = 5, [6] = 7 };

int main(void) {
    unsigned i, acc;
    struct Rec base = { .d = 4, .b = 2, .c = 3, .a = 1 };

    acc = (unsigned)(base.a + base.b + base.c + base.d);
    acc += (unsigned)seed_row[0] + (unsigned)seed_row[4] + (unsigned)seed_row[6];
    for (i = 0; i < N; i++) {
        struct Rec r = (struct Rec){
            .a = (int)i + base.a,
            .c = base.c,
            .b = (int)(i & 15u),
            .d = base.d
        };
        int tmp[4] = { [1] = r.b, [0] = r.a, [3] = r.d, [2] = r.c };
        acc += (unsigned)tmp[0] + (unsigned)tmp[1] + (unsigned)tmp[2] + (unsigned)tmp[3];
        base.a = r.b;
    }
    runtime.printf("init %u\n", acc);
    return 0;
}
