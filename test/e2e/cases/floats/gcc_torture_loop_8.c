// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-8.c
package main;

double a[3] = { 0.0, 1.0, 2.0 };

int bar (int x, double *y)
{
  if (x || *y != 1.0)
    return 1;

    return 0;}

int main ()
{
  double c;
  int d;
  for (d = 0; d < 3; d++)
  {
    c = a[d];
    if (c > 0.0) goto e;
  }
  bar(1, &c);
  return 1;
e:
  bar(0, &c);
  return 0;
}