// expect: 0
package main;

/* PR middle-end/71626 */

extern void abort (void);

typedef long V __attribute__((__vector_size__(sizeof (long))));

V
foo (void)
{
  V v = { (long) foo };
  return v;
}

int
main (void)
{
  V v = foo ();
  if (v[0] != (long) foo)
    abort ();
  return 0;
}
