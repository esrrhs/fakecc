// expect: 0
// unsupported_on: darwin-arm64
// 1.1897e+4932L only fits in quad/x87 long double; on arm64 Darwin, where
// long double is an 8-byte double, the literal rounds to +infinity and the
// test aborts.  The host clang behaves identically.
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
