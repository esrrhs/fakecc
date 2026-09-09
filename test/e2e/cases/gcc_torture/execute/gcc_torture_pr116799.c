/* PR rtl-optimization/116799 */
// expect: 0
package main;

extern unsigned long strlen(const char*);

const char *l;

__attribute__((noipa)) void
foo (const char *x, const char *y, int z)
{
  if (x != l + 1 || y != x || z)
    __builtin_abort ();
}

__attribute__((noipa)) void
bar (const char *x, char *v)
{
  const char *w = x + strlen (x);

  while (x[0] == '*' && x < w - 1)
    x++;

  const char *y = w - 1;
  int z = 1;
  if (y >= x)
    {
      while (y - x > 0 && *y == '*')
	y--;
      z = 0;
    }
  int i = 0;
  if (z)
    v[i++] = 'a';
  v[i] = 'b';
  foo (x, y, z);
}

int
main ()
{
  char v[2] = { 0 };
  l = "**";
  bar (l, v);
}
