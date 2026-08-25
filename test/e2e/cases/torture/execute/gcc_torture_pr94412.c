// expect: 0
package main;

/* PR middle-end/94412 */

extern void abort (void);

typedef unsigned V __attribute__ ((__vector_size__ (sizeof (unsigned) * 2)));

void
foo (V *v, V *w)
{
  *w = -*v / 11;
}

void
bar (V *v, V *w)
{
  *w = -18 / -*v;
}

int
main (void)
{
  V a = (V) { 1, 0 };
  V b = (V) { 3, 0x7fffffff };
  V c, d;
  foo (&a, &c);
  bar (&b, &d);
  if (c[0] != -1U / 11 || c[1] != 0 || d[0] != 0 || d[1] != -18U / -0x7fffffffU)
    abort ();
  return 0;
}
