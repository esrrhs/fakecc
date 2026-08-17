// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960116-1.c
package main;

static inline int
p (int *p)
{
  return !((long) p & 1);
}

int
f (int *q)
{
  if (p (q) && *q)
    return 1;
  return 0;
}

int
main (void)
{
  if (f ((int*) 0xffffffff) != 0)
    return 1;
  return 0;
}