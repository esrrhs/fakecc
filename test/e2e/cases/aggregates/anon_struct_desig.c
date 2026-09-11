// expect: 0
// Designators may name fields of an unnamed nested struct/union, matching
// member-access lookup.  Named nested structs still do not export fields.
package main;

struct S {
    struct {
        int a;
        int b;
    };
    int c;
};

union U {
    struct {
        int a;
        int b;
    };
    int x;
};

int main(void) {
    struct S s = { .a = 1, .b = 2, .c = 3 };
    if (s.a != 1) return 1;
    if (s.b != 2) return 2;
    if (s.c != 3) return 3;

    struct S t = { .c = 9, .a = 4, .b = 5 };
    if (t.a != 4) return 4;
    if (t.b != 5) return 5;
    if (t.c != 9) return 6;

    struct S p = { {1, 2}, 3 };
    if (p.a != 1) return 7;
    if (p.b != 2) return 8;
    if (p.c != 3) return 9;

    s.a = 10;
    if (s.a != 10) return 10;

    union U u = { .a = 5 };
    if (u.a != 5) return 11;

    union U v = { .x = 7 };
    if (v.x != 7) return 12;

    return 0;
}
