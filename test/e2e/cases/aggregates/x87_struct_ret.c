// expect: 0
// SysV: a 16-byte X87+X87UP aggregate returns in st0, not via sret.
// Passing it is still MEMORY (stack).  A 32-byte `{ long double, long double }`
// stays sret.
package main;

struct Ld { long double x; };
struct Inner { long double x; };
struct Nest { struct Inner i; };
union Uld { long double x; char b[16]; };
struct Pair { long double a; long double b; };

__attribute__((noinline))
struct Ld make_ld(long double v) {
    struct Ld s;
    s.x = v;
    return s;
}

__attribute__((noinline))
long double take_ld(struct Ld s) {
    return s.x;
}

__attribute__((noinline))
struct Nest make_nest(long double v) {
    struct Nest n;
    n.i.x = v;
    return n;
}

__attribute__((noinline))
union Uld make_u(long double v) {
    union Uld u;
    u.x = v;
    return u;
}

__attribute__((noinline))
struct Pair make_pair(long double a, long double b) {
    struct Pair p;
    p.a = a;
    p.b = b;
    return p;
}

int main(void) {
    struct Ld a = make_ld(3.0L);
    if (take_ld(a) != 3.0L) return 1;
    if (make_ld(4.5L).x != 4.5L) return 2;

    struct Nest n = make_nest(5.0L);
    if (n.i.x != 5.0L) return 3;

    union Uld u = make_u(6.0L);
    if (u.x != 6.0L) return 4;

    struct Pair p = make_pair(1.0L, 2.0L);
    if (p.a != 1.0L) return 5;
    if (p.b != 2.0L) return 6;

    struct Ld b;
    b.x = 7.0L;
    if (take_ld(b) != 7.0L) return 7;
    return 0;
}
