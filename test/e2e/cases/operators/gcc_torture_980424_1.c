// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980424-1.c
package main;

int i, a[99];

int f (int one)
{
  if (one != 1)
    return 1;

    return 0;}

void
g ()
{
  f (a[i & 0x3f]);
}

int
main ()
{
  a[0] = 1;
  i = 0x40;
  g ();
  return 0;
}