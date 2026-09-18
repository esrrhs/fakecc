// expect: 0
// aligned_call must allocate outgoing 32-byte MEMORY args from the current
// rsp.  lea from the fixed frame lands on a live VLA.
package main;
typedef double V __attribute__((vector_size(16)));
struct __attribute__((aligned(32))) A { V x; };

__attribute__((noinline))
int take(struct A a) {
    return (int)a.x[0];
}

int main(void) {
    int n = 32;
    int buf[n];
    int i;
    for (i = 0; i < n; i++)
        buf[i] = 100 + i;
    buf[0] = 42;
    buf[n - 1] = 99;
    {
        struct A a;
        a.x[0] = 7.0;
        a.x[1] = 8.0;
        if (take(a) != 7) return 1;
    }
    if (buf[0] != 42) return 2;
    if (buf[n - 1] != 99) return 3;
    if (buf[8] != 108) return 4;
    return 0;
}
