// expect: 0
package main;

/* PR target/64979 */

extern void abort (void);

typedef __builtin_va_list va_list;

void
bar (int x, va_list *ap)
{
  if (ap)
    {
      int i;
      for (i = 0; i < 10; i++)
	if (i != __builtin_va_arg (*ap, int))
	  abort ();
      if (__builtin_va_arg (*ap, double) != 0.5)
	abort ();
    }
}

void
foo (int x, ...)
{
  va_list ap;
  int n;

  __builtin_va_start (ap, x);
  n = __builtin_va_arg (ap, int);
  bar (x, (va_list *) ((n == 0) ? ((void *) 0) : &ap));
  __builtin_va_end (ap);
}

int
main (void)
{
  foo (100, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0.5);
  return 0;
}
