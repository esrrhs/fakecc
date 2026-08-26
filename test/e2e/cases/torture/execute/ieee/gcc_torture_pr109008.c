// expect: 0
package main;
__attribute__((noipa)) double
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
  if (foo (((double)2.22044604925031308084726333618164062e-16L) / 8.0) == 0.0)
    __builtin_abort ();
  return 0;
}
