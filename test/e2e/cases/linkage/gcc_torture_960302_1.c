// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960302-1.c
// Tests static local variable persistence across calls
package main;

int counter() {
    static int n = 0;
    return ++n;
}

int main() {
    if (counter() != 1) return 1;
    if (counter() != 2) return 2;
    if (counter() != 3) return 3;
    return 0;
}
