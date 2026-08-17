// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20060412-1.c
package main;

struct S
{
  long o;
};

struct T
{
  long o;
  struct S m[82];
};

struct T t;

int
main ()
{
  struct S *p, *q;

  p = (struct S *) &t;
  p = &((struct T *) p)->m[0];
  q = p + 82;
  while (--q > p)
    q->o = -1;
  q->o = 0;

  if (q > p)
    return 1;
  if (q - p > 0)
    return 1;
  return 0;
}