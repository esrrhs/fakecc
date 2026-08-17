// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000313-1.c
package main;

unsigned int buggy (unsigned int *param)
{
  unsigned int accu, zero = 0, borrow;
  accu    = - *param;
  borrow  = - (accu > zero);
  *param += accu;
  return borrow;
}

int main (void)
{
  unsigned int param  = 1;
  unsigned int borrow = buggy (&param);

  if (param != 0)
    return 1;
  if (borrow + 1 != 0)
    return 1;
  return 0;
}