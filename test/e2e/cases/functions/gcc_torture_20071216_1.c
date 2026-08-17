// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20071216-1.c
package main;

/* PR rtl-optimization/34490 */

static int x;

int

bar (void)
{
  return x;
}

int
foo (void)
{
  long int b = bar ();
  if ((unsigned long) b < -4095L)
    return b;
  if (-b != 38)
    b = -2;
  return b + 1;
}

int
main (void)
{
  x = 26;
  if (foo () != 26)
    return 1;
  x = -39;
  if (foo () != -1)
    return 1;
  x = -38;
  if (foo () != -37)
    return 1;
  return 0;
}