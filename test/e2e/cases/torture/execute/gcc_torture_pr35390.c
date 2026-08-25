// expect: 0
package main;

extern void abort(void);

unsigned int foo (int n)
{
  return ~((unsigned int)~n);
}

int main(void)
{
  if (foo(0) != 0)
    abort ();
  return 0;
}
