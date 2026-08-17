// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991202-2.c
package main;

int
f1 ()
{
  unsigned long x, y = 1;

  x = ((y * 8192) - 216) % 16;
  return x;
}

int
main ()
{
  if (f1 () != 8)
    return 1;
  return 0;
}