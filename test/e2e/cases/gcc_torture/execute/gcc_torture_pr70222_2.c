/* PR rtl-optimization/70222 */

// expect: 0
package main;

__attribute__((noinline)) unsigned int
foo (int x)
{
  unsigned long long y = -1ULL >> x;
  return (unsigned int) y >> 31;
}

int
main (void)
{
  if (foo (15) != 1 || foo (32) != 1 || foo (33) != 0)
    __builtin_abort ();
  return 0;
}
