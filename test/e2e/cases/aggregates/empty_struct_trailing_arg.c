// expect: 0
// GNU empty structs occupy no argument slots.  A later stack argument must
// still arrive intact, and sizeof of an empty struct is 0.
package main;
struct E {};
int take(int a, int b, int c, int d, int e, int f, struct E z, int h) {
    (void)z;
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return h;
}
int main(void) {
    if (sizeof(struct E) != 0) return 1;
    struct E z;
    if (take(1, 2, 3, 4, 5, 6, z, 42) != 42) return 2;
    return 0;
}
