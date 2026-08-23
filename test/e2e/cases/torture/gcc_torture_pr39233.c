// expect: 0
package main;

extern void abort(void);

__attribute__((noinline)) void
foo (void *p)
{
  long l = (long) p;
  if (l < 0 || l > 6)
    abort ();
}

int
main (void)
{
  short i;
  for (i = 6; i >= 0; i--)
    foo ((void *) (long) i);
  return 0;
}
