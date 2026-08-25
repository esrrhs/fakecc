/* PR tree-optimization/54471 */

// expect: 0
package main;

extern void abort(void);

__attribute__ ((noinline))
unsigned long long
foo (unsigned long long ixi, unsigned ctr)
{
  unsigned long long irslt = 1;
  unsigned long long ix = ixi;

  for (; ctr; ctr--)
    {
      irslt *= ix;
      ix *= ix;
    }

  if (irslt != 14348907)
    abort ();
  return irslt;
}

int
main (void)
{
  unsigned long long res;

  res = foo (3, 4);
  return 0;
}
