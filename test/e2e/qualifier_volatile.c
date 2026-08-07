// expect: 0
// `volatile` qualifier: parse a volatile-qualified variable, assign through a
// pointer (volatile must not break codegen), and read it back.  Returns 0 on
// success or a failing sentinel.
package main;
int main() {
    volatile int x = 5;
    int *p = (int *)&x;
    *p = 42;
    if (x != 42) return 1;
    return 0;
}
