// expect: 0
package main;

extern void abort(void);

long long __attribute__((noinline))
foo (long long t)
{
  while (t > -4)
    t -= 2;

  return t;
}

int main(void)
{
  if (foo (0) != -4)
    abort ();
  return 0;
}
