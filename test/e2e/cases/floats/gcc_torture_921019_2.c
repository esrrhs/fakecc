// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921019-2.c
package main;

int
main(void)
{
  double x,y=0.5;
  x=y/0.2;
  if(x!=x)
    return 1;
  return 0;
}