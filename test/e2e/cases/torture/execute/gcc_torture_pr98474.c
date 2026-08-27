// expect: 0
package main;

/* PR tree-optimization/98474 */

extern void abort (void);

typedef unsigned __int128 T;
enum { N = 64 };

void
foo (T *x)
{
  *x += ((T) 1) << (N + 1);
}

int
main (void)
{
  T a = ((T) 1) << (N + 1);
  T b = a;
  T n;
  foo (&b);
  n = b;
  while (n >= a)
    n -= a;
  if ((int) (n >> N) != 0)
    abort ();
  return 0;
}
