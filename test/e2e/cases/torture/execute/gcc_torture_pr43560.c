/* PR tree-optimization/43560 */

// expect: 0
package main;

struct S
{
  int a, b;
  char c[10];
};

__attribute__ ((noinline)) void
test (struct S *x)
{
  while (x->b > 1 && x->c[x->b - 1] == '/')
    {
      x->b--;
      x->c[x->b] = '\0';
    }
}

int
main (void)
{
  struct S s = { 0, 0, "" };
  struct S *p = &s;
  test (p);
  return 0;
}
