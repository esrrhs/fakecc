// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950511-1.c
package main;

int
main (void)
{
  unsigned long long xx;
  unsigned long long *x = (unsigned long long *) &xx;

  *x = -3;
  *x = *x * *x;
  if (*x != 9)
    return 1;
  return 0;
}