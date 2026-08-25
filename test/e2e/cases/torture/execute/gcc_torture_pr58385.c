/* PR tree-optimization/58385 */

// expect: 0
package main;

extern void abort(void);

int a, b = 1;

int
foo (void)
{
  b = 0;
  return 0;
}

int
main (void)
{
  ((0 || a) & foo () >= 0) <= 1 && 1;
  if (b)
    abort ();
  return 0;
}
