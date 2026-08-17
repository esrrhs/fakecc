// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930622-2.c
package main;

long double
ll_to_ld (long long n)
{
  return n;
}

long long
ld_to_ll (long double n)
{
  return n;
}

int
main (void)
{
  long long n;

  if (ll_to_ld (10LL) != 10.0)
    return 1;

  if (ld_to_ll (10.0) != 10)
    return 1;

  return 0;
}