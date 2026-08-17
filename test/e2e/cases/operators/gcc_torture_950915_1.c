// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950915-1.c
package main;

long int a = 100000;
long int b = 21475;

long
f ()
{
  return ((long long) a * (long long) b) >> 16;
}

int
main (void)
{
  if (f () < 0)
    return 1;
  return 0;
}