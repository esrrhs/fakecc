// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20080529-1.c
package main;

/* PR target/36362 */

int
test (float c)
{
  return !!c * 7LL == 0;
}

int
main (void)
{
  if (test (1.0f) != 0)
    return 1;
  return 0;
}