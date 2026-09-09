// expect: 0
package main;

extern void abort(void);

double d = 1.17549435e-38 / 2.0;

int main(void)
{
  double x = 1.17549435e-38 / 2.0;
  if (x != d)
    abort ();
  return 0;
}
