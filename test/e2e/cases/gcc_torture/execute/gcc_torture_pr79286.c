// expect: 0
package main;

/* PR tree-optimization/79286 */

int a = 0, c = 0;
static int d[][8] = {};

int
main (void)
{
  int e;
  for (int b = 0; b < 4; b++)
    {
      while (a && c++)
	e = d[300000000000000000][0];
    }

  return 0;
}
