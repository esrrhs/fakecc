// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021119-1.c
package main;

/* PR 8639.  */

int foo (int i)
{
  int r;
  r = (80 - 4 * i) / 20;
  return r;
}
    
int main ()
{
  if (foo (1) != 3)
    return 1;
  return 0;
}