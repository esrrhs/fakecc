/* PR tree-optimization/57321 */

// expect: 0
package main;

int a = 1, *b, **c;

static int
foo (int *p)
{
  if (*p == a)
    {
      int *i[7][5];
      int **j[1][1];
      i[0][0] = 0;
      j[0][0] = &i[0][0];
      *b = &p != c;
    }
  return 0;
}

int
main (void)
{
  int i = 0;
  foo (&i);
  return 0;
}
