/* PR tree-optimization/58277 */

// expect: 0
package main;

extern void abort(void);

static int a[1], b, c, e, i, j, k, m, q[] = { 1, 1 }, t;
int volatile d;
int **r;
static int ***volatile s = &r;
int f, g, o, x;
static int *volatile h = &f, *p;
char n;

static int
fn2(void)
{
  n = 0;
  for (; g; t++)
    {
      for (;; m++)
	{
	  d;
	  if (*p)
	    break;
	  return 0;
	}
      *h = 0;
    }
  return 1;
}

static void
fn3(void)
{
  if (fn2())
    {
      int *z[6];
      for (; n < 1; n++)
	*h = 0;
      int t1[7];
      for (; c; c++)
	o = t1[0];
    }
  *s = 0;
  for (n = 0;; n = 0)
    {
      int t4 = 0;
      if (q[n])
	break;
      *r = &t4;
    }
}

int
main(void)
{
  for (; j; j--)
    a[0] = 0;
  fn3();
  for (; k; k++)
    ;
 
  if (n)
    abort();

  return 0;
}
