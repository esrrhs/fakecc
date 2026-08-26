// expect: 0
package main;

/* PR tree-optimization/65053 */

extern void abort (void);

int i;
unsigned int x;

int
main (void)
{
  unsigned int n = x;
  unsigned int u = 32;
  if (n >= 32)
    abort ();
  if (n != 0)
    u = n + 32;

  while (u != 32)
    {
      u = 32;
      i = 1;
    }

  if (i)
    abort ();
  return 0;
}
