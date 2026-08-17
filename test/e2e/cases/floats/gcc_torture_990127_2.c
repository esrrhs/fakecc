// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990127-2.c
package main;

/* { dg-options "-mpc64"  { target { i?86-*-* x86_64-*-* } } } */

int
fpEq (double x, double y)
{
  if (x != y)
    return 1;

    return 0;}

void
fpTest (double x, double y)
{
  double result1 = (35.7 * 100.0) / 45.0;
  double result2 = (x * 100.0) / y;
  fpEq (result1, result2);
}

int
main ()
{
  fpTest (35.7, 45.0);
  return 0;
}