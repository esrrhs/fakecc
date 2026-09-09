// expect: 0
package main;

/* PR target/65956 */

extern void abort (void);

struct A { char *a; int b; long long c; };
char v[3];

void
fn1 (char *x, char *y)
{
  if (x != &v[1] || y != &v[2])
    abort ();
  v[1]++;
}

int
fn2 (char *x)
{
  return x == &v[0];
}

void
fn3 (const char *x)
{
  if (x[0] != 0)
    abort ();
}

static struct A
foo (const char *x, struct A y, struct A z)
{
  struct A r = { 0, 0, 0 };
  if (y.b && z.b)
    {
      if (fn2 (y.a) && fn2 (z.a))
	switch (x[0])
	  {
	  case '|':
	    break;
	  default:
	    fn3 (x);
	  }
      fn1 (y.a, z.a);
    }
  return r;
}

int
bar (int x, struct A *y)
{
  switch (x)
    {
    case 219:
      foo ("+", y[-2], y[0]);
    case 220:
      foo ("-", y[-2], y[0]);
    }
  return 0;
}

int
main (void)
{
  struct A a[3] = { { &v[1], 1, 1LL }, { &v[0], 0, 0LL }, { &v[2], 2, 2LL } };
  bar (220, a + 2);
  if (v[1] != 1)
    abort ();
  return 0;
}
