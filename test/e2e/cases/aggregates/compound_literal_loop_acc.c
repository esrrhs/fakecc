// expect: 0
// Designated compound-literal and array init in a loop (init bench shape).
// ADDR of the temps must fold into [rbp+disp] stores; values stay correct.
package main;

struct Rec { int a, b, c, d; };

int main(void) {
    unsigned i, acc;
    struct Rec base = { .d = 4, .b = 2, .c = 3, .a = 1 };
    acc = (unsigned)(base.a + base.b + base.c + base.d);
    for (i = 0; i < 20; i = i + 1) {
        struct Rec r = (struct Rec){
            .a = (int)i + base.a,
            .c = base.c,
            .b = (int)(i & 15u),
            .d = base.d
        };
        int tmp[4] = { [1] = r.b, [0] = r.a, [3] = r.d, [2] = r.c };
        acc += (unsigned)tmp[0] + (unsigned)tmp[1] + (unsigned)tmp[2]
             + (unsigned)tmp[3];
        base.a = r.b;
    }
    /* Hand-computed from the same recurrence. */
    if (acc != 590) return 1;
    return 0;
}
