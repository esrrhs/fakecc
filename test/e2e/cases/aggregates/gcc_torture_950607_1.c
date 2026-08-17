// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950607-1.c
package main;

int
main (void)
{
  struct { long status; } h;

  h.status = 0;
  if (((h.status & 128) == 1) && ((h.status & 32) == 0))
    return 1;
  return 0;
}