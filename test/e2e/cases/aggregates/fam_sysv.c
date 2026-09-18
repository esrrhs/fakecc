// expect: 0
// SysV classifies only the fixed prefix of a FAM / `[0]` struct.  The
// flexible tail is not MEMORY, and `{ char x[0]; }` occupies no slots.
package main;

struct F { int n; char d[]; };
struct Z { char x[0]; };
struct Z0 { int n; char d[0]; };

__attribute__((noinline))
int take_f(struct F f, int k) {
    return f.n + k;
}

__attribute__((noinline))
struct F make_f(int n) {
    struct F f;
    f.n = n;
    return f;
}

__attribute__((noinline))
int take6z(int a, int b, int c, int d, int e, int f, struct Z z, int h) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    (void)z;
    return h;
}

int pick_f(int n, ...) {
    va_list ap;
    struct F f;
    int k;
    va_start(ap, n);
    f = va_arg(ap, struct F);
    k = va_arg(ap, int);
    va_end(ap);
    return f.n + k;
}

int pick_z(int n, ...) {
    va_list ap;
    struct Z z;
    int h;
    va_start(ap, n);
    z = va_arg(ap, struct Z);
    h = va_arg(ap, int);
    va_end(ap);
    (void)z;
    return h;
}

int take_z0(struct Z0 f, int k) {
    return f.n + k;
}

int main(void) {
    struct F f;
    struct Z z;
    struct Z0 z0;
    struct F r;

    if (sizeof(struct F) != 4) return 1;
    if (sizeof(struct Z) != 0) return 2;
    if (sizeof(struct Z0) != 4) return 3;

    f.n = 1;
    if (take_f(f, 7) != 8) return 4;

    r = make_f(42);
    if (r.n != 42) return 5;

    if (take6z(1, 2, 3, 4, 5, 6, z, 99) != 99) return 6;

    f.n = 3;
    if (pick_f(0, f, 4) != 7) return 7;
    if (pick_z(0, z, 99) != 99) return 8;

    z0.n = 1;
    if (take_z0(z0, 7) != 8) return 9;
    return 0;
}
