/* PR target/44942 */

// expect: 0
package main;

void
test1 (int a, int b, int c, int d, int e, int f, int g, long double h, ...)
{
  int i;
  va_list ap;

  va_start (ap, h);
  i = va_arg (ap, int);
  if (i != 1234)
    __builtin_abort ();
  va_end (ap);
}

void
test3 (double a, double b, double c, double d, double e, double f,
       double g, long double h, ...)
{
  double i;
  va_list ap;

  va_start (ap, h);
  i = va_arg (ap, double);
  if (i != 1234.0)
    __builtin_abort ();
  va_end (ap);
}

int
main (void)
{
  test1 (0, 0, 0, 0, 0, 0, 0, 0.0L, 1234);
  test3 (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0L, 1234.0);
  return 0;
}
