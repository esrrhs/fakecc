// expect: 0
package main;

int a, d, f;
char g;
int c = 1;

int
foo (void)
{
  if (c == 0)
    return -1;
  return 1;
}

void
bar (int h, int i, char *k, char *m)
{
  for (; d < i; d += 2)
    for (int j = 0; j < h; j++)
      m[j] = k[4 * j];
}

void
baz (long h)
{
  char n = 0;
  bar (h, 4, &n, &g);
}

int
main (void)
{
  f = foo ();
  baz ((unsigned char) f - 4);
  return 0;
}
