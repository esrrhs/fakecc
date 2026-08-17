// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980604-1.c
package main;

int a = 1;
int b = -1;

int c = 1;
int d = 0;

int
main (void)
{
  double e;
  double f;
  double g;

  f = c;
  g = d;
  e = (a < b) ? f : g;
  if (e)
    return 1;
  return 0;
}