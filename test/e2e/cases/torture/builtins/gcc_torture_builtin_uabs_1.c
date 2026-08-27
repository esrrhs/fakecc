// expect: 0
// flags: -fno-builtin-uabs
package main;

/* Port of gcc.c-torture/execute/builtins/uabs-1.c
 * (+ uabs-1-lib.c + lib/main.c). -fno-builtin-uabs so uabs is a real call
 * (sets abs_called) while ulabs stays a builtin. */

extern void abort (void);

int abs_called = 0;
int inside_main = 0;

unsigned int
uabs (int x)
{
  if (inside_main)
    abs_called = 1;
  return (x < 0 ? -(unsigned int) x : x);
}

unsigned long
ulabs (long x)
{
  if (inside_main)
    abort ();
  return (x < 0 ? -(unsigned long) x : x);
}

extern unsigned int uabs (int);
extern unsigned long ulabs (long);

void
main_test (void)
{
  if (ulabs (0) != 0)
    abort ();
  if (uabs (0) != 0)
    abort ();
  if (!abs_called)
    abort ();
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}
