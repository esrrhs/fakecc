/* PR tree-optimization/48717 */

// expect: 0
package main;

extern void abort(void);

int v = 1, w;

unsigned short
foo (unsigned short x, unsigned short y)
{
  return x + y;
}

void
bar (void)
{
  v = foo (~w, w);
}

int
main (void)
{
  bar ();
  if (v != (unsigned short) -1)
    abort ();
  return 0;
}
