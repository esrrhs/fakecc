// expect: 0
package main;

/* PR target/36332 */

extern void abort(void);

int
foo (long double ld)
{
  return ld == __builtin_infl ();
}

int
main ()
{
  if (foo (1.18973149535723176502e+4932L))
    abort ();
  return 0;
}
