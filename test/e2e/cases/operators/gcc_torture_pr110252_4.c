// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr110252-4.c
package main;

int a, b = 2, c = 2;
int main() {
  b = ~(1 % (a ^ (b - (1 && c) || c & b)));
  if (b < -1)
    return 1;
  return 0;
}