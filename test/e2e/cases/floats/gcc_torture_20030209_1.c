// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030209-1.c
package main;

/* { dg-require-stack-size "8*100*100" } */

double x[100][100];
int main ()
{
  int i;

  i = 99;
  x[i][0] = 42;
  if (x[99][0] != 42)
    return 1;
  return 0;
}