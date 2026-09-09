// expect: 0
package main;

/* PR tree-optimization/65053 */

extern void abort (void);

int i;

unsigned int
foo (void)
{
  return 0;
}

int
main (void)
{
  unsigned int u = -1;
  if (u == -1)
    {
      unsigned int n = foo ();
      if (n > 0)
	u = n - 1;
    }

  while (u != -1)
    {
      u = -1;
      i = 1;
    }

  if (i)
    abort ();
  return 0;
}
