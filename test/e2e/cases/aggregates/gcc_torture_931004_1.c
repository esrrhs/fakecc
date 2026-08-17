// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/931004-1.c
package main;

struct tiny
{
  int c;
};

int
f (int n, struct tiny x, struct tiny y, struct tiny z, long l)
{
  if (x.c != 10)
    return 1;

  if (y.c != 11)
    return 1;

  if (z.c != 12)
    return 1;

  if (l != 123)
    return 1;

    return 0;}

int
main (void)
{
  struct tiny x[3];
  x[0].c = 10;
  x[1].c = 11;
  x[2].c = 12;
  f (3, x[0], x[1], x[2], (long) 123);
  return 0;
}