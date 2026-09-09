// expect: 0
package main;

int x;

void __attribute__((noinline)) foo (void)
{
  x = -x;
}
void __attribute__((noinline)) bar (void)
{
}

int __attribute__((noinline))
test (int c)
{
  int tmp = x;
  (c ? foo : bar) ();
  return tmp + x;
}

extern void abort(void);

int main(void)
{
  x = 1;
  if (test (1) != 0)
    abort ();
  return 0;
}
