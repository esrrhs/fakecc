// expect: 0
package main;

/* PR rtl-optimization/68328 */

extern void abort (void);

int a, b, c = 1, d = 1, e;

int
foo (void)
{
  return 4195552;
}

void
bar (int x, int y)
{
  if (y == 0)
    abort ();
}

int
baz (int x)
{
  char g, h;
  int i, j;

  foo ();
  for (;;)
    {
      if (c)
	h = d;
      g = h < x ? h : 0;
      i = (signed char) ((unsigned char) (g - 120) ^ 1);
      j = i > 97;
      if (a - j)
	bar (0x123456, 0);
      if (!b)
	return e;
    }
}

int
main (void)
{
  baz (2);
  return 0;
}
