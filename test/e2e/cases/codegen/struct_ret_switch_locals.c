// expect: 47
// INTEGER-class struct returns into independent named locals (switch) and
// an x87 long-double pair into a named local must keep their payloads.
//  use_many(3)==4, ld.x+ld.y==5, g[1].c+g[1].a==20+20; 4+5+20+20-2=47.
package main;

typedef struct { int a; int b; long long c; } S;
typedef struct { long double x; long double y; } LD;

static S mk(long long x) {
    S s;
    s.a = (int)x;
    s.b = 0;
    s.c = x;
    return s;
}

static LD mkld(long double a, long double b) {
    LD s;
    s.x = a;
    s.y = b;
    return s;
}

static long long use_many(int k) {
    S a, b, c, d, e, f, g, h, i;
    switch (k) {
    case 0: a = mk(1); return a.c;
    case 1: b = mk(2); return b.c;
    case 2: c = mk(3); return c.c;
    case 3: d = mk(4); return d.c;
    case 4: e = mk(5); return e.c;
    case 5: f = mk(6); return f.c;
    case 6: g = mk(7); return g.c;
    case 7: h = mk(8); return h.c;
    default: i = mk(9); return i.c;
    }
}

S g[2];

int main(void) {
    LD t;
    g[0] = mk(10);
    g[1] = mk(20);
    t = mkld(2.0L, 3.0L);
    return (int)(use_many(3) + (long long)(t.x + t.y) + g[1].c + g[1].a - 2);
}
