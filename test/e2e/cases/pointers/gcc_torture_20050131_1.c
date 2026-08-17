// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20050131-1.c
package main;

/* Verify that we do not lose side effects on a MOD expression.  */

int
foo (int a)
{
  int x = 0 % a++;
  return a;
}

int
main(void)
{
  if (foo (9) != 10)
    return 1;
  return 0;
}