// expect: 0
package main;
struct Big { long a; long b; long c; };
struct Big make_big(void) { struct Big s; s.a = 1; s.b = 2; s.c = 3; return s; }
int main(void) { struct Big s = make_big(); return (int)(s.a + s.b + s.c - 6); }
