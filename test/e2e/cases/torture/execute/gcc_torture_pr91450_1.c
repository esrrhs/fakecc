// expect: 0
package main;

/* PR middle-end/91450 */

extern void abort (void);

unsigned long long
foo (int a, int b)
{
  unsigned long long r;
  if (!__builtin_mul_overflow (a, b, &r))
    abort ();
  return r;
}

unsigned long long
bar (int a, int b)
{
  unsigned long long r;
  if (a >= 0)
    return 0;
  if (!__builtin_mul_overflow (a, b, &r))
    abort ();
  return r;
}

unsigned long long
baz (int a, int b)
{
  unsigned long long r;
  if (b >= 0)
    return 0;
  if (!__builtin_mul_overflow (a, b, &r))
    abort ();
  return r;
}

unsigned long long
qux (int a, int b)
{
  unsigned long long r;
  if (a >= 0)
    return 0;
  if (b < 0)
    return 0;
  if (!__builtin_mul_overflow (a, b, &r))
    abort ();
  return r;
}

unsigned long long
quux (int a, int b)
{
  unsigned long long r;
  if (a < 0)
    return 0;
  if (b >= 0)
    return 0;
  if (!__builtin_mul_overflow (a, b, &r))
    abort ();
  return r;
}

int
main (void)
{
  if (foo (-4, 2) != -8ULL)
    abort ();
  if (foo (2, -4) != -8ULL)
    abort ();
  if (bar (-4, 2) != -8ULL)
    abort ();
  if (baz (2, -4) != -8ULL)
    abort ();
  if (qux (-4, 2) != -8ULL)
    abort ();
  if (quux (2, -4) != -8ULL)
    abort ();
  if (foo (-2, 1) != -2ULL)
    abort ();
  if (foo (1, -2) != -2ULL)
    abort ();
  if (bar (-2, 1) != -2ULL)
    abort ();
  if (baz (1, -2) != -2ULL)
    abort ();
  if (qux (-2, 1) != -2ULL)
    abort ();
  if (quux (1, -2) != -2ULL)
    abort ();
  return 0;
}
