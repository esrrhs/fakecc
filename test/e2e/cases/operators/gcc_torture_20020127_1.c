// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020127-1.c
package main;

/* This used to fail on h8300.  */

unsigned long
foo (unsigned long n)
{
  return (~n >> 3) & 1;
}

int
main ()
{
  if (foo (1 << 3) != 0)
    return 1;

  if (foo (0) != 1)
    return 1;

  return 0;
}