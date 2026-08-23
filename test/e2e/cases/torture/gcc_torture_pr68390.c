// expect: 0
package main;

__attribute__ ((noinline))
double direct(int x, ...)
{
  return x*x;
}

__attribute__ ((noinline))
double broken(double (*indirect)(int x, ...), int v)
{
  return indirect(v);
}

int main (void)
{
  double d1;
  int i = 2;
  d1 = broken (direct, i);
  if (d1 != i*i)
  {
    __builtin_abort ();
  }
  return 0;
}
