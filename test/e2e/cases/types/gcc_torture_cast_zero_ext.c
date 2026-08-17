// expect: 0
// Ported from GCC C-Torture suite: gcc.c-torture/execute/20000412-1.c
package main;

int main() {
    short s = -1;
    unsigned int u = (unsigned short)s;
    if (u != 65535) return 1;
    return 0;
}
