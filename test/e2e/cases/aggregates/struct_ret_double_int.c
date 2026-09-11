// expect: 0
// Mixed SSE+INTEGER struct return: {double; int} travels in xmm0 + rax.
package main;
struct DI { double d; int a; };
struct DI make_di(void) {
    struct DI s;
    s.d = 4.0;
    s.a = 3;
    return s;
}
int main(void) {
    struct DI s = make_di();
    if (s.d != 4.0) return 1;
    if (s.a != 3) return 2;
    return 0;
}
