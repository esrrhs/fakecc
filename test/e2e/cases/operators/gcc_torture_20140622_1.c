// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20140622-1.c
package main;

unsigned p;

long 
test (unsigned a)
{
  return (long)(p + a) - (long)p;
}

int
main ()
{
  p = (unsigned) -2;
  if (test (0) != 0)
    return 1;
  if (test (1) != 1)
    return 1;
  if (test (2) != -(long)(unsigned)-2)
    return 1;
  p = (unsigned) -1;
  if (test (0) != 0)
    return 1;
  if (test (1) != -(long)(unsigned)-1)
    return 1;
  if (test (2) != -(long)(unsigned)-2)
    return 1;
  return 0;
}