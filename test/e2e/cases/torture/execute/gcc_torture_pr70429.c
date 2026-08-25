/* PR rtl-optimization/70429 */

// expect: 0
package main;

__attribute__((noinline)) int
foo (int a)
{
  return (int) (0x14ff6e2207db5d1fLL >> a) >> 4;
}

int
main (void)
{
  if (foo (1) != 0x3edae8 || foo (2) != -132158092)
    __builtin_abort ();
  return 0;
}
