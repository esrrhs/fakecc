// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040831-1.c
package main;

/* This testcase was being miscompiled, because operand_equal_p
   returned that (unsigned long) d and (long) d are equal.  */

int
main (void)
{
  double d = -12.0;
  long l = (d > 10000) ? (unsigned long) d : (long) d;
  if (l != -12)
    return 1;
  return 0;
}