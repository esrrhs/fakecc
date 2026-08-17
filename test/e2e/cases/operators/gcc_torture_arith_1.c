// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/arith-1.c
package main;

int main() {
    int a = 7, b = 3;
    if (a + b != 10) return 1;
    if (a - b != 4)  return 2;
    if (a * b != 21) return 3;
    if (a / b != 2)  return 4;
    if (a % b != 1)  return 5;
    return 0;
}
