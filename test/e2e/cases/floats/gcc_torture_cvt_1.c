// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/cvt-1.c
package main;

static inline long
g1 (double x)
{
  return (double) (long) x;
}

long
g2 (double f)
{
  return f;
}

double
f (long i)
{
  if (g1 (i) != g2 (i))
    return 1;
  return g2 (i);
}

int
main (void)
{
  if (f (123456789L) != 123456789L)
    return 1;
  if (f (123456789L) != g2 (123456789L))
    return 1;
  return 0;
}