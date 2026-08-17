// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930929-1.c
// Tests negative modulo and division for signed integers
package main;

int main() {
    if ((-7) / 2 != -3) return 1;
    if ((-7) % 2 != -1) return 2;
    if (7 / (-2) != -3) return 3;
    if (7 % (-2) != 1)  return 4;
    return 0;
}
