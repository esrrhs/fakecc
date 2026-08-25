// expect: 0
package main;

int __attribute__ ((noinline))
foo (void)
{
  return 123;
}

int __attribute__ ((noinline))
bar (void)
{
  int c = 1;
  c |= 4294967295U ^ (foo () | 4073709551608U);
  return c;
}

int
main (void)
{
  if (bar () != 0x83fd4005)
    __builtin_abort ();
  return 0;
}
