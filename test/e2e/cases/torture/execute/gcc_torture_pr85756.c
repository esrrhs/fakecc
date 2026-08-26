// expect: 0
package main;

/* PR target/85756 */

int a, c, *e, f, h = 10;
short b;
unsigned int p;

void
bar (int a)
{
}

void
foo (void)
{
  unsigned j = 1, m = 430523;
  int k, n = 1, *l = &h;
lab:
  p = m;
  m = -((~65535U | j) - n);
  f = b << ~(n - 8);
  n = (m || b) ^ f;
  j = p;
  if (p < m)
    *l = k < 3;
  if (!n)
    l = &k;
  if (c)
    {
      bar (a);
      goto lab;
    }
  if (!*l)
    *e = 1;
}

int
main (void)
{
  foo ();
  return 0;
}
