// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021127-1.c
package main;

/* { dg-options "-std=c99" } */

long long a = -1;
long long llabs (long long);

int
main()
{
  if (llabs (a) != 1)
    return 1;
  return 0;
}
long long llabs (long long b)
{
	return 1;
}