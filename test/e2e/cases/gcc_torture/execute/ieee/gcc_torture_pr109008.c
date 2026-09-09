// expect: 0
package main;

/* PR tree-optimization/109008 */

extern void abort(void);

double
foo (double eps)
{
  double d = 1. + eps;
  if (d == 1.)
    return eps;
  return 0.0;
}

int
main ()
{
  if (foo (2.2204460492503131e-16 / 8.0) == 0.0)
    abort ();
  return 0;
}
