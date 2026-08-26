// expect: 0
// skip_difftest
// Host GCC still has tree-optimization/123864: __builtin_mul_overflow_p
// (long long, unsigned, long long) misses overflow, so gcc aborts and the
// oracle disagrees with the expected GCC 16 behavior this port checks.
package main;
[[gnu::noipa]] static int
foo (long long x)
{
  return __builtin_mul_overflow_p (x, ~0U, x);
}
int
main ()
{
  if (foo (0))
    __builtin_abort ();
  if (foo (0x7fffffff + 1LL))
    __builtin_abort ();
  if (!foo (0x7fffffff + 2LL))
    __builtin_abort ();
  if (foo (-0x7fffffff - 1LL))
    __builtin_abort ();
  if (!foo (-0x7fffffff - 2LL))
    __builtin_abort ();
}
