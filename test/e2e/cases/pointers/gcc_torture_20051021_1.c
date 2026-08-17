// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20051021-1.c
package main;

/* Verify that TRUTH_AND_EXPR is not wrongly changed to TRUTH_ANDIF_EXPR.  */

int count = 0;

int foo1(void)
{
  count++;
  return 0;
}

int foo2(void)
{
  count++;
  return 0;
}

int main(void)
{
  if ((foo1() == 1) & (foo2() == 1))
    return 1;

  if (count != 2)
    return 1;

  return 0;
}