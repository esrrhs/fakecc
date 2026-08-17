// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010925-1.c
package main;

int foo(int x, int y) {
    if ((x & 0xFF) == (y & 0xFF)) return 1;
    return 0;
}

int main() {
    if (foo(0x1234, 0x5634) != 1) return 1;
    if (foo(0x1234, 0x5678) != 0) return 2;
    return 0;
}
