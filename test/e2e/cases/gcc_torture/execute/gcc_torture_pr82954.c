// expect: 0
package main;

/* PR tree-optimization/82954 */

extern void abort (void);

void
foo (int *p, int *q)
{
  p[0] = p[0] ^ 1;
  p[1] = p[1] ^ 2;
  p[2] = p[2] ^ q[2];
  p[3] = p[3] ^ q[3];
}

int
main (void)
{
  int p[4] = { 16, 32, 64, 128 };
  int q[4] = { 8, 4, 2, 1 };
  foo (p, q);
  if (p[0] != 17 || p[1] != 34 || p[2] != 66 || p[3] != 129)
    abort ();
  return 0;
}
