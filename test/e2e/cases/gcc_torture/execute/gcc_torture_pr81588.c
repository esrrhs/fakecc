// expect: 0
package main;

/* PR tree-optimization/81588 */

extern void abort (void);

int
bar (int x)
{
  return x;
}

int
foo (unsigned x, long long y)
{
  if (y < 0)
    return 0;
  if (y < (long long) (4 * x))
    {
      bar (y);
      return 1;
    }
  return 0;
}

int
main (void)
{
  unsigned x = 10;
  long long y = -10000;
  if (foo (x, y) != 0)
    abort ();
  y = -1;
  if (foo (x, y) != 0)
    abort ();
  y = 0;
  if (foo (x, y) != 1)
    abort ();
  y = 39;
  if (foo (x, y) != 1)
    abort ();
  y = 40;
  if (foo (x, y) != 0)
    abort ();
  y = 10000;
  if (foo (x, y) != 0)
    abort ();
  return 0;
}
