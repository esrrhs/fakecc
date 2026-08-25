/* PR rtl-optimization/79388 */

// expect: 0
package main;

unsigned int a, c;

__attribute__ ((noinline)) unsigned int
foo (unsigned int p)
{
  p |= 1;
  p &= 0xfffe;
  p %= 0xffff;
  c = p;
  return a + p;
}

int
main (void)
{
  int x = foo (6);
  if (x != 6)
    __builtin_abort();
  return 0;
}
