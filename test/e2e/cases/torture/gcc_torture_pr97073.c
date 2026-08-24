/* PR middle-end/97073 */

// expect: 0
package main;

typedef unsigned long long L;
union U { L i; struct T { unsigned k; L l; } j; } u;

__attribute__((noinline)) void
foo (L x)
{
  u.j.l = u.i & x;
}

int
main (void)
{
  u.i = 5;
  foo (-1ULL);
  if (u.j.l != 5)
    __builtin_abort ();
  return 0;
}
