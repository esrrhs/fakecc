// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr106032.c
package main;

/* PR rtl-optimization/106032 */

 int
foo (int x, int *y)
{
  int a = 0;
  if (x < 0)
    a = *y;
  return a;  
}

int
main ()
{
  int a = 42;
  if (foo (0, 0) != 0 || foo (1, 0) != 0)
    return 1;
  if (foo (-1, &a) != 42 || foo (-42, &a) != 42)
    return 1;
  return 0;
}