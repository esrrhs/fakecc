/* PR tree-optimization/63302 */

// expect: 0
package main;

__attribute__((noinline)) int
bar (long long x)
{
  long long v = x & (((long long) -1 << 31) | 0x7ff);
 
  return v == 0 || v == ((long long) -1 << 31);
}

int
main (void)
{
  if (bar (0) != 1
      || bar (1) != 0
      || bar (0x800) != 1
      || bar (0x801) != 0
      || bar (1LL << 31) != 0
      || bar (-1LL << 31) != 1
      || bar ((-1LL << 31) | 1) != 0
      || bar ((-1LL << 31) | 0x800) != 1
      || bar ((-1LL << 31) | 0x801) != 0)
    __builtin_abort ();
  return 0;
}
