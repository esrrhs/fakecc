// expect: 0
package main;

extern void abort(void);

_Complex float __attribute__ ((noinline)) foo (_Complex float x)
{
  return x;
}

_Complex float __attribute__ ((noinline)) bar (_Complex float x)
{
  return foo (x);
}

int main(void)
{
  _Complex float a, b;
  __real__ a = 9;
  __imag__ a = 42;

  b = bar (a);

  if (a != b)
    abort ();

  return 0;
}
