/* PR tree-optimization/97325 */

// expect: 0
package main;

unsigned long long
foo (unsigned long long c)
{
  return c ? __builtin_ffs (-(unsigned short) c) : 0;
}

int
main (void)
{
  if (foo (2) != 2)
    __builtin_abort ();
  return 0;
}
