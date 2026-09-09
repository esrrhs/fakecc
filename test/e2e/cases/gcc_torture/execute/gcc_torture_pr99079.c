// expect: 0
package main;

/* PR tree-optimization/99079 */

extern void abort (void);

unsigned long long
foo (int x)
{
  unsigned long long s = 1 << x;
  return 4897637220ULL % s;
}

int
main ()
{
  if (foo (31) != 4897637220ULL)
    abort ();
  return 0;
}
