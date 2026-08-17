// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020916-1.c
package main;

/* Distilled from try_pre_increment in flow.c.  If-conversion inserted
   new instructions at the wrong place on ppc.  */

int foo(int a)
{
  int x;
  x = 0;
  if (a > 0) x = 1;
  if (a < 0) x = 1;
  return x;
}

int main()
{
  if (foo(1) != 1)
    return 1;
  return 0;
}