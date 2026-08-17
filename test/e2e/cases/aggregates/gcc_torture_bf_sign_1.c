// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/bf-sign-1.c
package main;

int
main (void)
{
  struct  {
    signed int s:3;
    unsigned int u:3;
    int i:3;
  } x = {-1, -1, -1};

  if (x.u != 7)
    return 1;
  if (x.s != - 1)
    return 1;

  if (x.i != -1 && x.i != 7)
    return 1;

  return 0;
}