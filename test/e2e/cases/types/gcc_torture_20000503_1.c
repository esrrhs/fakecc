// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000503-1.c
package main;

unsigned long
sub (int a)
{
  return ((0 > a - 2) ? 0 : a - 2) * sizeof (long);
}

int
main (void)
{
  if (sub (0) != 0)
    return 1;

  return 0;
}