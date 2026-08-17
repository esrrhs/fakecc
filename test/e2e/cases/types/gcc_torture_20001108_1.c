// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001108-1.c
package main;

long long
signed_poly (long long sum, long x)
{
  sum += (long long) (long) sum * (long long) x;
  return sum;
}

unsigned long long
unsigned_poly (unsigned long long sum, unsigned long x)
{
  sum += (unsigned long long) (unsigned long) sum * (unsigned long long) x;
  return sum;
}

int
main (void)
{
  if (signed_poly (2LL, -3) != -4LL)
    return 1;
  
  if (unsigned_poly (2ULL, 3) != 8ULL)
    return 1;

  return 0;
}