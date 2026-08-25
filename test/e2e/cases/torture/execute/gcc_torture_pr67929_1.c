// expect: 0
package main;

int __attribute__ ((noinline))
foo (float a)
{
  return a * 4.9f;
}

int
main (void)
{
  if (foo (10.0f) != 49)
    __builtin_abort ();

  return 0;
}
