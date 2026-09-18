// expect: 0
// GNU empty structs return in no slot and must not take a hidden sret.
// If sret stole RDI, `x` would not be 42.
package main;

struct E {};

static int bad;

struct E make(int x) {
    struct E e;
    if (x != 42) bad = 1;
    return e;
}

int take(struct E z, int y) {
    (void)z;
    return y;
}

int main(void) {
    struct E e = make(42);
    if (bad) return 1;
    if (take(e, 7) != 7) return 2;
    if (take(make(42), 9) != 9) return 3;
    if (bad) return 4;
    return 0;
}
