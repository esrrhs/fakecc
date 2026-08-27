// expect: 0
package main;

/* PR tree-optimization/94524 */

extern void abort (void);

typedef signed char __attribute__ ((__vector_size__ (16))) V;

static V
foo (V c)
{
  c %= (signed char) -19;
  return (V) c;
}

int
main (void)
{
  V x = foo ((V) { 31 });
  if (x[0] != 12)
    abort ();
  return 0;
}
