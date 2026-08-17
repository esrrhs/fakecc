// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/931004-9.c
package main;

struct tiny
{
  char c;
  char d;
};

int
f (int n, struct tiny x, struct tiny y, struct tiny z, long l)
{
  if (x.c != 10)
    return 1;
  if (x.d != 20)
    return 1;

  if (y.c != 11)
    return 1;
  if (y.d != 21)
    return 1;

  if (z.c != 12)
    return 1;
  if (z.d != 22)
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
  x[0].d = 20;
  x[1].d = 21;
  x[2].d = 22;
  f (3, x[0], x[1], x[2], (long) 123);
  return 0;
}