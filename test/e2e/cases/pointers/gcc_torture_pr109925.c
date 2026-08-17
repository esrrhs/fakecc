// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr109925.c
package main;

/* PR tree-optimization/109925 */

int a, c, f;

int
main ()
{
  int g[2];
  for (c = 0; c < 2; c++)
    {
      {
	char h[20], *b = h;
	int d = 48, e = 0;
	while (d && e < 5)
	  b[e++] = d /= 10;
	f = e;
      }
      g[f - 2 + c] = 0;
    }
  for (;;)
    {
      for (; a <= 4; a++)
	if (g[0])
	  break;
      break;
    }
  if (a != 5)
    return 1;
  return 0;
}