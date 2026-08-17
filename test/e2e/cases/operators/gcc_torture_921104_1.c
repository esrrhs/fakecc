// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921104-1.c
package main;

int
main (void)
{
  unsigned long val = 1;

  if (val > (unsigned long) ~0)
    return 1;
  return 0;
}