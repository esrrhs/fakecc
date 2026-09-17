// expect: 0
// Trailing NO_CLASS padding is not an INTEGER register.  aligned(16)
// `{ long x; }` is one GP; the next int stays in the next GP (esi), not rdx.
package main;

struct __attribute__((aligned(16))) S { long x; };
struct __attribute__((aligned(16))) D { double d; };

__attribute__((noinline))
int take_s(struct S s, int k) {
    return (int)s.x + k;
}

__attribute__((noinline))
int take_d(struct D s, int k) {
    return (int)s.d + k;
}

int main(void) {
    struct S s;
    struct D d;
    s.x = 3;
    if (take_s(s, 4) != 7) return 1;
    d.d = 3.0;
    if (take_d(d, 4) != 7) return 2;
    if (sizeof(struct S) != 16) return 3;
    return 0;
}
