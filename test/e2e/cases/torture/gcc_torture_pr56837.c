// expect: 0
package main;

extern void abort(void);

_Complex int a[1024];

__attribute__((noinline)) void
foo (void)
{
  int i;
  for (i = 0; i < 1024; i++)
    a[i] = -1;
}

int
main (void)
{
  int i;
  foo ();
  for (i = 0; i < 1024; i++)
    if (a[i] != -1)
      abort ();
  return 0;
}
