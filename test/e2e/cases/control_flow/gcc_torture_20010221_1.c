// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010221-1.c
package main;

int n = 2;

int
main (void)
{
  int i, x = 45;

  for (i = 0; i < n; i++)
    {
      if (i != 0)
	x = ( i > 0 ) ? i : 0;
    }

  if (x != 1)
    return 1;
  return 0;
}