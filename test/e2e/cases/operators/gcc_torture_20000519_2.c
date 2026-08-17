// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000519-2.c
package main;

long x = -1L;

int main()
{
  long b = (x != -1L);

  if (b)
    return 1;

  return 0;
}