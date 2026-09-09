// expect: 0
package main;

/* PR target/71554 */

extern void abort (void);

int v;

void
bar (void)
{
  v++;
}

void
foo (unsigned int x)
{
  signed int y = ((-2147483647 - 1) / 2);
  signed int r;
  if (__builtin_mul_overflow (x, y, &r))
    bar ();
}

int
main (void)
{
  foo (2);
  if (v)
    abort ();
  return 0;
}
