// expect: 0
package main;

/* PR tree-optimization/54471 */

extern void abort (void);

typedef long long T;
typedef unsigned long long UT;

UT
foo (T ixi, unsigned ctr)
{
  UT irslt = 1;
  T ix = ixi;

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
  UT res;

  res = foo (3, 4);
  return 0;
}
