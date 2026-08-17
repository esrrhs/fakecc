// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960219-1.c
package main;

int
f (int i)
{
  if (((1 << i) & 1) == 0)
    return 1;

    return 0;}

int
main (void)
{
  f (0);
  return 0;
}