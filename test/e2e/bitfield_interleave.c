// expect: 0
// A normal member between bitfield runs closes the open unit; both sides work.
package main;
struct S {
    unsigned a : 4;
    int mid;
    unsigned b : 4;
};
int main(void) {
    struct S s;
    s.a = 3;
    s.mid = 100;
    s.b = 5;
    if (s.a != 3) return 1;
    if (s.mid != 100) return 2;
    if (s.b != 5) return 3;
    s.a = 0xff;
    if (s.a != 15) return 4;
    if (s.mid != 100) return 5;
    if (s.b != 5) return 6;
    s.mid = 7;
    if (s.a != 15) return 7;
    if (s.b != 5) return 8;
    return 0;
}
