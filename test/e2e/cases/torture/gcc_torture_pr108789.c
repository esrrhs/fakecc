// expect: 0
package main;
int
add (unsigned *r, const unsigned *a, const unsigned *b)
{
  return __builtin_add_overflow (*a, *b, r);
}
int
mul (unsigned *r, const unsigned *a, const unsigned *b)
{
  return __builtin_mul_overflow (*a, *b, r);
}
int
main ()
{
  unsigned x;
  x = (0x7fffffff + 1U) / 2;
  if (add (&x, &x, &x))
    __builtin_abort ();
  x = 1U << (sizeof (int) * 8 / 4);
  if (mul (&x, &x, &x))
    __builtin_abort ();
  x = 0x7fffffff + 1U;
  if (!add (&x, &x, &x))
    __builtin_abort ();
  x = 1U << (sizeof (int) * 8 / 2);
  if (!mul (&x, &x, &x))
    __builtin_abort ();
}
