// expect: 0
/* Bitfield inserts of a constant: both `value` and the width mask are
 * literals.  Skipping both left the value register undefined (930126-1,
 * pr58570, pr57344). */
package main;

struct s {
    unsigned long long a : 8, b : 32;
};

struct s set_b(struct s x) {
    x.b = 0xcdef1234;
    return x;
}

struct __attribute__((packed)) P {
    int f0 : 15;
    int f1 : 29;
};

int e = 1, i;
static struct P d[6];

int main(void) {
    static struct s t;
    t.a = 12;
    t = set_b(t);
    if (t.a != 12 || t.b != 0xcdef1234)
        return 1;
    if (e) {
        d[i].f0 = 1;
        d[i].f1 = 1;
    }
    if (d[0].f1 != 1)
        return 2;
    return 0;
}
