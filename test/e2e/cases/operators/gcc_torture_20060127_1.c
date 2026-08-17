// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20060127-1.c
package main;

int
f (long long a)
{
  if ((a & 0xffffffffLL) != 0)
    return 1;

    return 0;}

long long a = 0x1234567800000000LL;

int
main ()
{
  f (a);
  return 0;
}