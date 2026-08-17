// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960321-1.c
package main;

char a[10] = "deadbeef";

char
acc_a (long i)
{
  return a[i-2000000000L];
}

int
main (void)
{
  if (acc_a (2000000000L) != 'd')
    return 1;
  return 0;
}