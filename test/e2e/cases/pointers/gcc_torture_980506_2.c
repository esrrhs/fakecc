// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980506-2.c
package main;

static void *self(void *p){ return p; }

int
f()
{
  struct { int i; } s, *sp;
  int *ip = &s.i;

  s.i = 1;
  sp = self(&s);
  
  *ip = 0;
  return sp->i+1;
}

int
main(void)
{
  if (f () != 1)
    return 1;
  else
    return 0;
}