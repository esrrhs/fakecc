// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040706-1.c
package main;

int main ()
{
  int i;
  for (i = 0; i < 10; i++)
    continue;
  if (i < 10)
    return 1;
  return 0;
}