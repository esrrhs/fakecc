// expect: 0
package main;

/* PR tree-optimization/60960 */

extern void abort (void);
extern int memcmp (const void *, const void *, unsigned long);

typedef unsigned char v4qi __attribute__ ((vector_size (4)));

static v4qi
f1 (v4qi v)
{
  return v / 2;
}

static v4qi
f2 (v4qi v)
{
  return v / (v4qi) { 2, 2, 2, 2 };
}

static v4qi
f3 (v4qi x, v4qi y)
{
  return x / y;
}

int
main (void)
{
  v4qi x = { 5, 5, 5, 5 };
  v4qi y = { 2, 2, 2, 2 };
  v4qi z = f1 (x);
  if (memcmp (&y, &z, sizeof (y)) != 0)
    abort ();
  z = f2 (x);
  if (memcmp (&y, &z, sizeof (y)) != 0)
    abort ();
  z = f3 (x, y);
  if (memcmp (&y, &z, sizeof (y)) != 0)
    abort ();
  return 0;
}
