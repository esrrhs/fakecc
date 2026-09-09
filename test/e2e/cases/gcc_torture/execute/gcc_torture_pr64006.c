// expect: 0
package main;

/* PR tree-optimization/64006 */

extern void abort (void);

int v;

long
test (long *x, int y)
{
  int i;
  long s = 1;
  for (i = 0; i < y; i++)
    if (__builtin_mul_overflow (s, x[i], &s))
      v++;
  return s;
}

int
main (void)
{
  long d[7] = { 975, 975, 975, 975, 975, 975, 975 };
  long r = test (d, 7);
  if (sizeof (long) * 8 == 64 && v != 1)
    abort ();
  else if (sizeof (long) * 8 == 32 && v != 4)
    abort ();
  return 0;
}
