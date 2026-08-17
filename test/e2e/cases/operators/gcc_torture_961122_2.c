// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/961122-2.c
package main;

int
f (int a)
{
  return ((a >= 0 && a <= 10) && ! (a >= 0));
}

int
main (void)
{
  if (f (0))
    return 1;
  return 0;
}