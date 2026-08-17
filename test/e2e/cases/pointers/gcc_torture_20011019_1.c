// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20011019-1.c
package main;

struct { int a; int b[5]; } x;
int *y;

int foo (void)
{
  return y - x.b;
}

int main (void)
{
  y = x.b;
  if (foo ())
    return 1;
  return 0;
}