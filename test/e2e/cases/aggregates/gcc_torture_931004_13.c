// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/931004-13.c
package main;

struct tiny
{
  char c;
  char d;
  char e;
  char f;
};

int
f (int n, struct tiny x, struct tiny y, struct tiny z, long l)
{
  if (x.c != 10)
    return 1;
  if (x.d != 20)
    return 1;
  if (x.e != 30)
    return 1;
  if (x.f != 40)
    return 1;

  if (y.c != 11)
    return 1;
  if (y.d != 21)
    return 1;
  if (y.e != 31)
    return 1;
  if (y.f != 41)
    return 1;

  if (z.c != 12)
    return 1;
  if (z.d != 22)
    return 1;
  if (z.e != 32)
    return 1;
  if (z.f != 42)
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
  x[0].e = 30;
  x[1].e = 31;
  x[2].e = 32;
  x[0].f = 40;
  x[1].f = 41;
  x[2].f = 42;
  f (3, x[0], x[1], x[2], (long) 123);
  return 0;
}