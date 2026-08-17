// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000419-1.c
package main;

struct foo { int a, b, c; };

int
brother (int a, int b, int c)
{
  if (a)
    return 1;

    return 0;}

void
sister (struct foo f, int b, int c)
{
  brother ((f.b == b), b, c);
}

int
main ()
{
  struct foo f = { 7, 8, 9 };
  sister (f, 1, 2);
  return 0;
}