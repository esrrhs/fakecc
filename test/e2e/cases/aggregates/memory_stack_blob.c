// expect: 0
// SysV MEMORY aggregates are copied onto the outgoing stack (not a
// hidden pointer, not one IR eightbyte per 8 bytes of payload).
package main;
struct M { long a; long b; long c; };
struct Big { unsigned char x[200]; };

__attribute__((noinline))
struct M idm(struct M s) { return s; }

__attribute__((noinline))
long sum3(struct M s, int k) { return s.a + s.b + s.c + k; }

__attribute__((noinline))
int pick(struct Big b, int i) { return b.x[i]; }

int main(void) {
    struct M s;
    s.a = 1;
    s.b = 2;
    s.c = 4;
    struct M t = idm(s);
    if (t.a != 1) return 1;
    if (t.b != 2) return 2;
    if (t.c != 4) return 3;
    if (sum3(s, 3) != 10) return 4;
    struct Big b;
    int i;
    for (i = 0; i < 200; i++)
        b.x[i] = (unsigned char)i;
    if (pick(b, 0) != 0) return 5;
    if (pick(b, 199) != 199) return 6;
    return 0;
}
