// expect: 0
package main;

/* PR tree-optimization/94524 */

extern void abort (void);

typedef signed char __attribute__ ((__vector_size__ (16))) V;

static V
foo (V c)
{
  c %= (signed char) -128;
  return (V) c;
}

int
main (void)
{
  V x = foo ((V) { -128 });
  if (x[0] != 0)
    abort ();
  x = foo ((V) { -127 });
  if (x[0] != -127)
    abort ();
  x = foo ((V) { 127 });
  if (x[0] != 127)
    abort ();
  return 0;
}
