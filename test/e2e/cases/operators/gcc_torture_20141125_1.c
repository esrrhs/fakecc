// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20141125-1.c
package main;

int f(long long a) ;
int f(long long a)
{
  if (a & 0x3ffffffffffffffull)
    return 1;
  return 1024;
}

int main(void)
{
  if(f(0x48375d8000000000ull) != 1)
    return 1;
  if (f(0xfc00000000000000ull) != 1024)
    return 1;
  return 0;
}