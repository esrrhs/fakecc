/* PR rtl-optimization/80501 */

// expect: 0
package main;

signed char v = 0;

static signed char
foo (int x, int y)
{
  return x << y;
}

__attribute__((noinline)) int
bar (void)
{
  return foo (v >= 0, 8 - 1) >= 1;
}

int
main (void)
{
  if (sizeof (int) > sizeof (char) && bar () != 0)
    __builtin_abort ();
  return 0;
}
