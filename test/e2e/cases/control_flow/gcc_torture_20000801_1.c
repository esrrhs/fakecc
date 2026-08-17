// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000801-1.c
package main;

int foo(int x) {
    switch (x) {
    case 1: return 10;
    case 2: return 20;
    case 3: return 30;
    default: return -1;
    }
}

int main() {
    if (foo(1) != 10) return 1;
    if (foo(2) != 20) return 2;
    if (foo(3) != 30) return 3;
    if (foo(4) != -1) return 4;
    return 0;
}
