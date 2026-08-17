// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20060110-2.c
package main;

long long 
f (long long a, long long b) 
{ 
  return ((a + b) << 32) >> 32; 
} 

long long a = 0x1234567876543210LL;
long long b = 0x2345678765432101LL;
long long c = ((0x1234567876543210LL + 0x2345678765432101LL) << 32) >> 32;

int
main ()
{
  if (f (a, b) != c)
    return 1;
  return 0;
}