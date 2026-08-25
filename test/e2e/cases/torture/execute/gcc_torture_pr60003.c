/* PR tree-optimization/60003 */

// expect: 0
package main;

extern void abort(void);

unsigned long long jmp_buf[5];

__attribute__((noinline)) void
baz (void)
{
  __builtin_longjmp (jmp_buf, 1);
}

void
bar (void)
{
  baz ();
}

__attribute__((noinline)) int
foo (int x)
{
  int a = 0;

  if (__builtin_setjmp (jmp_buf) == 0)
    {
      while (1)
	{
	  a = 1;
	  bar ();
	}
    }
  else
    {
      if (a == 0)
	return 0;
      else
	return x;
    }
}

int
main (void)
{
  if (foo (1) == 0)
    abort ();

  return 0;
}
