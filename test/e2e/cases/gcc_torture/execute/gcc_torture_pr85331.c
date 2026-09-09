// expect: 0
package main;

/* PR tree-optimization/85331 */

extern void abort (void);

typedef double V __attribute__((vector_size (2 * sizeof (double))));
typedef long long W __attribute__((vector_size (2 * sizeof (long long))));

void
foo (V *r)
{
  V y = { 1.0, 2.0 };
  W m = { 10000000001LL, 0LL };
  *r = __builtin_shuffle (y, m);
}

int
main (void)
{
  V r;
  foo (&r);
  if (r[0] != 2.0 || r[1] != 1.0)
    abort ();
  return 0;
}
