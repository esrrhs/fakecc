// expect: 0
// Mixed INTEGER+SSE struct return: {int; double} travels in rax + xmm0.
package main;
struct ID { int a; double d; };
struct ID make_id(void) {
    struct ID s;
    s.a = 3;
    s.d = 4.0;
    return s;
}
int main(void) {
    struct ID s = make_id();
    if (s.a != 3) return 1;
    if (s.d != 4.0) return 2;
    return 0;
}
