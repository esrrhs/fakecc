/* PR tree-optimization/59014 */

// expect: 0
package main;

__attribute__((noinline)) long long int
foo (long long int x, long long int y)
{
  if (((int) x | (int) y) != 0)
    return 6;
  return x + y;
}

int
main (void)
{
  int shift_half = sizeof (int) * 8 / 2;
  long long int x = (3LL << shift_half) << shift_half;
  long long int y = (5LL << shift_half) << shift_half;
  long long int z = foo (x, y);
  if (z != ((8LL << shift_half) << shift_half))
    __builtin_abort ();
  return 0;
}
