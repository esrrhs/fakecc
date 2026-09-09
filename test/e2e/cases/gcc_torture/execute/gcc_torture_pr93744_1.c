/* PR tree-optimization/93744 */

// expect: 0
package main;

typedef int I;

int
main (void)
{
  int a = 0;
  I b = 0;
  (a > 0) * (b |= 2);
  if (b != 2)
    __builtin_abort ();
  return 0;
}
