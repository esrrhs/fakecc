// expect: 0
package main;

/* PR tree-optimization/69320 */

extern void abort (void);
extern void exit (int);

int a, *c, d, e, g, f;
short b;

int
fn1 (void)
{
  int h = d != 10;
  if (h > g)
    ;
  if (h == 10)
    {
      int *i = 0;
      a = 0;
      for (; a < 7; a++)
	for (; *i;)
	  ;
    }
  else
    {
      b = e / h;
      return f;
    }
  c = &h;
  abort ();
}

int
main (void)
{
  fn1 ();
  return 0;
}
