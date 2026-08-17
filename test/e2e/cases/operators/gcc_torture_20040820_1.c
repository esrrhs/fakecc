// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040820-1.c
package main;

/* PR rtl-optimization/17099 */

int
check (int a)
{
  if (a != 1)
    return 1;

    return 0;}

void
test (int a, int b)
{
  check ((a ? 1 : 0) | (b ? 2 : 0));
}

int
main (void)
{
  test (1, 0);
  return 0;
}