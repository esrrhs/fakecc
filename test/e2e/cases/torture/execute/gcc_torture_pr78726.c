// expect: 0
package main;

/* PR tree-optimization/78726 */

extern void abort (void);

unsigned char b = 36, c = 173;
unsigned int d;

void
foo (void)
{
  unsigned a = ~b;
  d = a * c * c + 1023094746U * a;
}

int
main (void)
{
  if (sizeof (int) != 4 || sizeof (char) != 1)
    return 0;
  foo ();
  if (d != 799092689U)
    abort ();
  return 0;
}
