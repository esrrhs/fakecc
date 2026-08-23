/* PR rtl-optimization/64957 */

// expect: 0
package main;

__attribute__((noinline)) int
foo (int b)
{
  return (((b ^ 5) | 1) ^ 5) | 1;
}

__attribute__((noinline)) int
bar (int b)
{
  return (((b ^ ~5) & ~1) ^ ~5) & ~1;
}

int
main (void)
{
  int i;
  for (i = 0; i < 16; i++)
    if (foo (i) != (i | 1) || bar (i) != (i & ~1))
      __builtin_abort ();
  return 0;
}
