// expect: 0
package main;
int
foo (long double ld)
{
  return ld == __builtin_infl ();
}
int
main ()
{
  if (foo (1.18973149535723176502126385303097021e+4932L))
    __builtin_abort ();
  return 0;
}
