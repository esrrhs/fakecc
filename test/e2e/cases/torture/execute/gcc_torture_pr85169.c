// expect: 0
package main;

/* PR target/85169 */

extern void abort (void);

typedef char V __attribute__((vector_size (64)));

static void
foo (V *p)
{
  V v = *p;
  v[63] = 1;
  *p = v;
}

int
main (void)
{
  V v = (V) { };
  foo (&v);
  for (unsigned i = 0; i < 64; i++)
    if (v[i] != (i == 63))
      abort ();
  return 0;
}
