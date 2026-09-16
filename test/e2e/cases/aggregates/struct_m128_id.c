// expect: 0
// `struct { 16-byte vector }` is SSE+SSEUP (one XMM), not two SSE registers.
package main;
typedef double V __attribute__((vector_size(16)));
struct S { V x; };
struct S id(struct S s) { return s; }
int main(void) {
    struct S a;
    a.x[0] = 3.0;
    a.x[1] = 4.0;
    struct S b = id(a);
    if (b.x[0] != 3.0) return 1;
    if (b.x[1] != 4.0) return 2;
    return 0;
}
