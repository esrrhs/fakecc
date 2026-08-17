// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010222-1.c
package main;

int a[2] = { 18, 6 };

int main ()
{
  int b = (-3 * a[0] -3 * a[1]) / 12;
  if (b != -6)
    return 1;
  return 0;
}