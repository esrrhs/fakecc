// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001112-1.c
package main;

int main ()
{
  long long i = 1;

  i = i * 2 + 1;
  
  if (i != 3)
    return 1;
  return 0;
}