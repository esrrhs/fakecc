// expect: 0
package main;

/* PR middle-end/91450 */

extern void abort (void);

void
foo (int a, int b)
{
  unsigned long long r;
  if (__builtin_mul_overflow (a, b, &r))
    abort ();
  if (r != 0)
    abort ();
}

void
bar (int a, int b)
{
  unsigned long long r;
  if (a >= 0)
    return;
  if (__builtin_mul_overflow (a, b, &r))
    abort ();
  if (r != 0)
    abort ();
}

void
baz (int a, int b)
{
  unsigned long long r;
  if (b >= 0)
    return;
  if (__builtin_mul_overflow (a, b, &r))
    abort ();
  if (r != 0)
    abort ();
}

void
qux (int a, int b)
{
  unsigned long long r;
  if (a >= 0)
    return;
  if (b < 0)
    return;
  if (__builtin_mul_overflow (a, b, &r))
    abort ();
  if (r != 0)
    abort ();
}

void
quux (int a, int b)
{
  unsigned long long r;
  if (a < 0)
    return;
  if (b >= 0)
    return;
  if (__builtin_mul_overflow (a, b, &r))
    abort ();
  if (r != 0)
    abort ();
}

int
main (void)
{
  foo (-4, 0);
  foo (0, -4);
  foo (0, 0);
  bar (-4, 0);
  baz (0, -4);
  qux (-4, 0);
  quux (0, -4);
  return 0;
}
