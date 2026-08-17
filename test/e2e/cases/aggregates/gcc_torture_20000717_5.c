// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000717-5.c
package main;

typedef struct trio { int a, b, c; } trio;

int
bar (int i, int j, int k, trio t)
{
  if (t.a != 1 || t.b != 2 || t.c != 3 ||
      i != 4 || j != 5 || k != 6)
    return 1;
}

int
foo (trio t, int i, int j, int k)
{
  return bar (i, j, k, t);
}

int
main (void)
{
  trio t = { 1, 2, 3 };

  foo (t, 4, 5, 6);
  return 0;
}