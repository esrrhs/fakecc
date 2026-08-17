// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20230509-1.c
package main;

int  f(unsigned a, int b)
{
  if (a < 0) ((void)0);
  if (a > 30) ((void)0);
  int t = a;
  if (b)  t = 100;
  else  if (a != 0)
    t = a ;
  else
    t = 1;
  return t;
}

int main(void)
{
  if (f(0, 0) != 1)
    return 1;
  if (f(1, 0) != 1)
    return 1;
  if (f(0, 1) != 100)
    return 1;
  if (f(1, 0) != 1)
    return 1;
  if (f(30, 0) != 30)
    return 1;
}