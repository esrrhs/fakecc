// expect: 0
package main;

/* PR tree-optimization/93945 */

extern void abort (void);
extern void *memset (void *, int, unsigned long);

union U { char a[8]; struct S { unsigned int b : 8, c : 13, d : 11; } e; } u;

int
foo (void)
{
  memset (&u.a, 0xf4, sizeof (u.a));
  return u.e.c;
}

int
bar (void)
{
  return u.e.c;
}

int
baz (void)
{
  memset (&u.a, 0xf4, sizeof (u.a));
  return u.e.d;
}

int
qux (void)
{
  return u.e.d;
}

int
main (void)
{
  int a = foo ();
  int b = bar ();
  if (a != b)
    abort ();
  if (a != 5364)
    abort ();
  a = baz ();
  b = qux ();
  if (a != b)
    abort ();
  if (a != 1959)
    abort ();
  return 0;
}
