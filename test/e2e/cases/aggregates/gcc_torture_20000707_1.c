// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000707-1.c
package main;

struct baz {
  int a, b, c;
};

int
foo (int a, int b, int c)
{
  if (a != 4)
    return 1;

    return 0;}

void
bar (struct baz x, int b, int c)
{
  foo (x.b, b, c);
}

int
main ()
{
  struct baz x = { 3, 4, 5 };
  bar (x, 1, 2);
  return 0;
}