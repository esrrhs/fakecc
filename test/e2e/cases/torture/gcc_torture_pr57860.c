/* PR rtl-optimization/57860 */

// expect: 0
package main;

extern void abort(void);

int a, *b = &a, c, d, e, *f = &e, g, *h = &d, k[1] = { 1 };

int
foo (int p)
{
  for (;; g++)
    {
      for (; c; c--);
      *f = *h = p > ((0x1FFFFFFFFLL ^ a) & *b);
      if (k[g])
	return 0;
    }
}

int
main (void)
{
  foo (1);
  if (d != 1)
    abort ();
  return 0;
}
