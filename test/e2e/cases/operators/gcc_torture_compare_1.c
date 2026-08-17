// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/compare-1.c
// Tests various integer comparison operators
package main;

int main() {
    int a = 5, b = 10;
    if (!(a < b))  return 1;
    if (!(a <= b)) return 2;
    if (a > b)     return 3;
    if (a >= b)    return 4;
    if (a == b)    return 5;
    if (!(a != b)) return 6;
    return 0;
}
