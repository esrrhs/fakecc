// expect: 0
package main;

/* PR middle-end/84521 */

extern void abort (void);

void
broken_longjmp (void *p)
{
  __builtin_longjmp (p, 1);
}

int x = 256;
void *p = (void*)&x;
void *p1;

void
test (void)
{
  void *buf[5];
  void *q = p;

  if (!__builtin_setjmp (buf))
    broken_longjmp (buf);

  /* Fails if stack pointer corrupted.  */
  if (p != q)
    abort ();
}

void
test2 (void)
{
  void *q = p;
  p1 = __builtin_alloca (x);
  test ();

  /* Fails if frame pointer corrupted.  */
  if (p != q)
    abort ();
}

int
main (void)
{
  void *q = p;
  test ();
  test2 ();
  /* Fails if stack pointer corrupted.  */
  if (p != q)
    abort ();

  return 0;
}
