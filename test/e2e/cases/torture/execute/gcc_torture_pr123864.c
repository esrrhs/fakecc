// expect: 0
// skip_difftest: host gcc miscompiles this case (upstream GCC PR 123864)
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
