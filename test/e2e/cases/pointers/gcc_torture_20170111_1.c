// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20170111-1.c
package main;

/* PR rtl-optimization/79032 */
/* Reported by Daniel Cederman <cederman@gaisler.com> */

struct S {
  short a;
  long long b;
  short c;
  char d;
  unsigned short e;
  long *f;
};

static long foo (struct S *s) ;

static long foo (struct S *s)
{
  long a = 1;
  a /= s->e;
  s->f[a]--;
  return a;
}

int main (void)
{
  long val = 1;
  struct S s = { 0, 0, 0, 0, 2, &val };
  val = foo (&s);
  if (val != 0)
    return 1;
  return 0;
}