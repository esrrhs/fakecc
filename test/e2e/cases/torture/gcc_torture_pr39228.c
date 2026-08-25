// expect: 0
package main;

extern void abort(void);

static int testf(float b)
{
  float c = 1.01f * b;
  return __builtin_isinff(c);
}

static int test(double b)
{
  double c = 1.01 * b;
  return __builtin_isinf(c);
}

int main(void)
{
  if (testf(3.40282347e+38f) < 1)
    abort();

  if (test(1.7976931348623157e+308) < 1)
    abort();

  return 0;
}
