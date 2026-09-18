// expect: 0
// A GNU union with a true FAM (`char d[]`) is MEMORY.  `[0]` is not a FAM
// and stays INTEGER like the fixed prefix.
package main;

union Ufam { int n; char d[]; };
union Uz { int n; char d[0]; };

__attribute__((noinline))
int take_fam(union Ufam u, int k) {
    return u.n + k;
}

__attribute__((noinline))
int take_z(union Uz u, int k) {
    return u.n + k;
}

__attribute__((noinline))
union Ufam make_fam(int n) {
    union Ufam u;
    u.n = n;
    return u;
}

__attribute__((noinline))
int take6_fam(int a, int b, int c, int d, int e, int f, union Ufam u, int h) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return u.n + h;
}

__attribute__((noinline))
int take6_z(int a, int b, int c, int d, int e, int f, union Uz u, int h) {
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f;
    return u.n + h;
}

int main(void) {
    union Ufam u;
    union Uz z;

    if (sizeof(union Ufam) != 4) return 1;
    if (sizeof(union Uz) != 4) return 2;

    u.n = 1;
    if (take_fam(u, 7) != 8) return 3;

    z.n = 1;
    if (take_z(z, 7) != 8) return 4;

    if (make_fam(42).n != 42) return 5;

    u.n = 3;
    if (take6_fam(1, 2, 3, 4, 5, 6, u, 9) != 12) return 6;

    z.n = 3;
    if (take6_z(1, 2, 3, 4, 5, 6, z, 9) != 12) return 7;
    return 0;
}
