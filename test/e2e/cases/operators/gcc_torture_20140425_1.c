// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20140425-1.c
package main;

/* PR target/60941 */
/* Reported by Martin Husemann <martin@netbsd.org> */

static void 
set (unsigned long *l)
{
  *l = 31;
}

int main (void)
{
  unsigned long l;
  int i;

  set (&l);
  i = (int) l;
  l = (unsigned long)(2U << i);
  if (l != 0)
    return 1;
  return 0;
}