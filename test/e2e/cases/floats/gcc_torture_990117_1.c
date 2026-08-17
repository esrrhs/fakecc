// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990117-1.c
package main;

int
foo (int x, int y, int i, int j)
{
  double tmp1 = ((double) x / y);
  double tmp2 = ((double) i / j);

  return tmp1 < tmp2;
}

int
main (void)
{
  if (foo (2, 24, 3, 4) == 0)
    return 1;
  return 0;
}