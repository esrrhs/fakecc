// expect: 0
package main;

/* PR rtl-optimization/84169 */

extern void abort (void);

typedef unsigned __int128 T;

T b;

static T
foo (T c, T d, T e, T f, T g, T h)
{
  __builtin_mul_overflow ((unsigned char) h, -16, &h);
  return b + h;
}

int
main (void)
{
  T x = foo (0, 0, 0, 0, 0, 4);
  if (x != (T) -64)
    abort ();
  return 0;
}
