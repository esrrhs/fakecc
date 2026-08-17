// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000717-1.c
package main;

typedef struct trio { int a, b, c; } trio;

int
bar (int i, trio t)
{
  if (t.a == t.b || t.a == t.c)
    return 1;
}

int
foo (trio t, int i)
{
  return bar (i, t);
}

int
main (void)
{
  trio t = { 1, 2, 3 };

  foo (t, 4);
  return 0;
}