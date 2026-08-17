// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040705-1.c
// Tests signed right shift preserves sign bit
package main;

int main() {
    int x = -8;
    if ((x >> 1) != -4) return 1;
    if ((x >> 2) != -2) return 2;
    if ((x >> 3) != -1) return 3;
    return 0;
}
