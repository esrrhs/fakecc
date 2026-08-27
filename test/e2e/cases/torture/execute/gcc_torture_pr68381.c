// expect: 0
package main;

/* PR rtl-optimization/68381 */

extern void abort (void);

int
foo (unsigned short x, unsigned short y)
{
  int r;
  if (__builtin_mul_overflow (x, y, &r))
    abort ();
  return r;
}

int
main (void)
{
  int x = 1;
  int y = 2;
  if (foo (x, y) != x * y)
    abort ();
  return 0;
}
