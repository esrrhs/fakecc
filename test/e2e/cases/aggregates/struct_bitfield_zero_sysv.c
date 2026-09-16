// expect: 0
// Zero-width `: 0` is not a SysV eightbyte; a following float stays in XMM.
package main;
struct S { int a; int : 0; float f; };
float id(struct S s) { return s.f; }
int main(void) {
    struct S s;
    s.a = 7;
    s.f = 42.0f;
    if (id(s) != 42.0f) return 1;
    if (s.a != 7) return 2;
    return 0;
}
