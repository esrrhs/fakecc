// expect: 0
// Bare 16-byte vectors and `struct { vector16 }` travel in one XMM (SSE+SSEUP).
package main;
typedef double V __attribute__((vector_size(16)));
struct S { V x; };

V pick_v(int n, ...) {
    va_list ap;
    va_start(ap, n);
    V v = va_arg(ap, V);
    va_end(ap);
    return v;
}

struct S pick_s(int n, ...) {
    va_list ap;
    va_start(ap, n);
    struct S s = va_arg(ap, struct S);
    va_end(ap);
    return s;
}

int main(void) {
    V a = { 1.5, 2.5 };
    V b = pick_v(0, a);
    if (b[0] != 1.5) return 1;
    if (b[1] != 2.5) return 2;
    struct S s;
    s.x = a;
    struct S t = pick_s(0, s);
    if (t.x[0] != 1.5) return 3;
    if (t.x[1] != 2.5) return 4;
    return 0;
}
