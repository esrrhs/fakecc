// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001221-1.c
package main;

int main ()
{
  unsigned long long a;
  if (! (a = 0xfedcba9876543210ULL))
    return 1;
  return 0;
}