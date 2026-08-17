// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20140828-1.c
package main;

short *f(short *a, int b, int *d) ;

short *f(short *a, int b, int *d)
{
  short c = *a;
  a++;
  c = b << c;
  *d = c;
  return a;
}

int main(void)
{
  int d;
  short a[2];
  a[0] = 0;
  if (f(a, 1, &d) != &a[1])
    return 1;
  if (d != 1)
    return 1;
  return 0;
}