// expect: 0
// flags: -fno-builtin-abs
package main;

/* Port of gcc.c-torture/execute/builtins/abs-1.c
 * (+ abs-1-lib.c + lib/main.c). -fno-builtin-abs so abs is a real call
 * (sets abs_called) while labs stays a builtin. */

extern void abort (void);

int abs_called = 0;
int inside_main = 0;

int
abs (int x)
{
  if (inside_main)
    abs_called = 1;
  return (x < 0 ? -x : x);
}

long
labs (long x)
{
  if (inside_main)
    abort ();
  return (x < 0 ? -x : x);
}

extern int abs (int);
extern long labs (long);

void
main_test (void)
{
  if (labs (0) != 0)
    abort ();
  if (abs (0) != 0)
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
