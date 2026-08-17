// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021204-1.c
package main;

/* This test was miscompiled when using sibling call optimization,
   because X ? Y : Y - 1 optimization changed X into !X in place
   and haven't reverted it if do_store_flag was successful, so
   when expanding the expression the second time it was
   !X ? Y : Y - 1.  */

int foo (int x)
{
  if (x != 1)
    return 1;

    return 0;}

int z;

int main (int argc, char **argv)
{
  char *a = "test";
  char *b = a + 2;

  foo (z > 0 ? b - a : b - a - 1);
  return 0;
}