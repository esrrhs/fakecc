// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20050124-1.c
package main;

/* PR rtl-optimization/19579 */

int
foo (int i, int j)
{
  int k = i + 1;

  if (j)
    {
      if (k > 0)
	k++;
      else if (k < 0)
	k--;
    }

  return k;
}

int
main (void)
{
  if (foo (-2, 0) != -1)
    return 1;
  if (foo (-1, 0) != 0)
    return 1;
  if (foo (0, 0) != 1)
    return 1;
  if (foo (1, 0) != 2)
    return 1;
  if (foo (-2, 1) != -2)
    return 1;
  if (foo (-1, 1) != 0)
    return 1;
  if (foo (0, 1) != 2)
    return 1;
  if (foo (1, 1) != 3)
    return 1;
  return 0;
}