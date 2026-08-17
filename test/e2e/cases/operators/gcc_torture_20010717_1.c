// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010717-1.c
package main;

int
main ()
{
  int i, j;
  unsigned long u, r1, r2;

  i = -16;
  j = 1;
  u = i + j;

  /* no sign extension upon shift */
  r1 = u >> 1;
  /* sign extension upon shift, but there shouldn't be */
  r2 = ((unsigned long) (i + j)) >> 1;

  if (r1 != r2)
    return 1;

  return 0;
}