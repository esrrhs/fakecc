// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921208-1.c
package main;

double
f(double x)
{
  return x*x;
}

double
Int(double (*f)(double), double a)
{
  return (*f)(a);
}

int
main(void)
{
  if (Int(&f,2.0) != 4.0)
    return 1;
  return 0;
}