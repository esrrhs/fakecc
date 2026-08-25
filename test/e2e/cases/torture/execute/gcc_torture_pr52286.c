// expect: 0
package main;

/* PR tree-optimization/52286 */

extern void abort (void);

int
main (void)
{
  int a = 0;
  int b = (~a | 1) & -2038094497;
  if (b >= 0)
    abort ();
  return 0;
}
