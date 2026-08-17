// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-11.c
package main;

static int a[199];

static void
foo ()
{
  int i;
  for (i = 198; i >= 0; i--)
    a[i] = i;
}

int
main ()
{
  int i;
  foo ();
  for (i = 0; i < 199; i++)
    if (a[i] != i)
      return 1;
  return 0;
}