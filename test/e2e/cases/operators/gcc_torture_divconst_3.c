// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/divconst-3.c
package main;

long long
f (long long x)
{
  return x / 10000000000LL;
}

int
main (void)
{
  if (f (10000000000LL) != 1 || f (100000000000LL) != 10)
    return 1;
  return 0;
}