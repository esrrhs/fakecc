/* PR target/64242 */

// expect: 0
package main;

extern void abort(void);

__attribute ((noinline)) void
broken_longjmp (void *p)
{
  void *buf[32];
  __builtin_memcpy (buf, p, 5 * sizeof (void*));
  __builtin_memset (p, 0, 5 * sizeof (void*));
  __builtin_longjmp (buf, 1);
}

volatile int x = 0;
char *volatile p;
char *volatile q;

int
main (void)
{
  void *buf[5];
  p = __builtin_alloca (x);
  q = __builtin_alloca (x);
  if (!__builtin_setjmp (buf))
    broken_longjmp (buf);

  p = q + (q - p);

  if (p != __builtin_alloca (x))
    abort ();

  return 0;
}
