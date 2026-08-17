// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr113010.c
package main;

int minus_1 = -1;

int
main ()
{
  if ((0, 0xffffffffull) >= minus_1)
    return 1;
  return 0;
}