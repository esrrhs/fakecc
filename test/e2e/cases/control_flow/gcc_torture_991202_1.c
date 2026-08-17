// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991202-1.c
package main;

int x, y;

int
main()
{
  x = 2;
  y = x;
  do
    {
      x = y;
      y = 2 * y;
    }
  while ( ! ((y - x) >= 20));
  return 0;
}