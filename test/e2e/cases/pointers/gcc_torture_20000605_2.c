// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000605-2.c
package main;

struct F { int i; };

int f1(struct F *x, struct F *y)
{
  int timeout = 0;
  for (; ((const struct F*)x)->i < y->i ; x->i++)
    if (++timeout > 5)
      return 1;

    return 0;}

int
main(void)
{
  struct F x, y;
  x.i = 0;
  y.i = 1;
  f1 (&x, &y);
  return 0;
}