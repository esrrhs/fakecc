// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950706-1.c
package main;

int
f (int n)
{
  return (n > 0) - (n < 0);
}

int
main (void)
{
  if (f (-1) != -1)
    return 1;
  if (f (1) != 1)
    return 1;
  if (f (0) != 0)
    return 1;
  return 0;
}