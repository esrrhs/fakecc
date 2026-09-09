// expect: 0
package main;

/* PR tree-optimization/65170 */

extern void abort (void);

typedef unsigned __int128 V;
typedef unsigned long long int H;

void
foo (V b, V c)
{
  V a;
  b &= (H) -1;
  c &= (H) -1;
  a = b * c;
  if (a != 1)
    abort ();
}

int
main (void)
{
  foo (1, 1);
  return 0;
}
