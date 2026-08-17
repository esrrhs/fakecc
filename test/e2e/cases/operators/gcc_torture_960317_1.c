// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960317-1.c
package main;

int
f (unsigned bitcount, int mant)
{
  int mask = -1 << bitcount;
  {
    if (! (mant & -mask))
      goto ab;
    if (mant & ~mask)
      goto auf;
  }
ab:
  return 0;
auf:
  return 1;
}

int
main (void)
{
  if (f (0, -1))
    return 1;
  return 0;
}