/* PR rtl-optimization/57829 */

// expect: 0
package main;

__attribute__((noinline))
int
f1 (int k)
{
  return 2 | ((k - 1) >> ((int) sizeof (int) * 8 - 1));
}

__attribute__((noinline))
long int
f2 (long int k)
{
  return 2L | ((k - 1L) >> ((int) sizeof (long int) * 8 - 1));
}

__attribute__((noinline))
int
f3 (int k)
{
  k &= 63;
  return 4 | ((k + 2) >> 5);
}

int
main (void)
{
  if (f1 (1) != 2 || f2 (1L) != 2L || f3 (63) != 6 || f3 (1) != 4)
    __builtin_abort ();
  return 0;
}
