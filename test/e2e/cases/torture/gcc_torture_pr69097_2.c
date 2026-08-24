/* PR tree-optimization/69097 */

// expect: 0
package main;

__attribute__((noinline)) int
f1 (int x, int y)
{
  return x % y;
}

__attribute__((noinline)) int
f2 (int x, int y)
{
  return x % -y;
}

__attribute__((noinline)) int
f3 (int x, int y)
{
  int z = -y;
  return x % z;
}

int
main (void)
{
  if (f1 (-2147483647 - 1, 1) != 0
      || f2 (-2147483647 - 1, -1) != 0
      || f3 (-2147483647 - 1, -1) != 0)
    __builtin_abort ();
  return 0;
}
