// expect: 0
// C99 6.7.8: a later nested designator overlays one subobject and must not
// wipe siblings already initialized by an earlier current-object designator.
package main;
struct Inner { int a; int b; };
struct Outer { struct Inner i; int x; };
struct T { int a; int b; int c; };
struct U { struct T t; };
int main(void) {
    struct Outer o = { .i = {1, 2}, .i.b = 7 };
    if (o.i.a != 1) return 1;
    if (o.i.b != 7) return 2;
    if (o.x != 0) return 3;
    struct U u = { .t.a = 1, .t.c = 3, .t.b = 2 };
    if (u.t.a != 1) return 4;
    if (u.t.b != 2) return 5;
    if (u.t.c != 3) return 6;
    struct Outer p = { .i.a = 4, .i.b = 5, .x = 6 };
    if (p.i.a != 4) return 7;
    if (p.i.b != 5) return 8;
    if (p.x != 6) return 9;
    return 0;
}
