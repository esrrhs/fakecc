// expect: 0
package main;

/* PR tree-optimization/85156 */

extern void abort (void);

int x, y;

int
foo (int z)
{
  if (__builtin_expect (x ? y != 0 : 0, z++))
    return 7;
  return z;
}

int
main (void)
{
  x = 1;
  if (foo (10) != 11)
    abort ();
  return 0;
}
