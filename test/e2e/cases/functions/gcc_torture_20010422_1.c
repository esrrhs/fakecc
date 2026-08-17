// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010422-1.c
package main;

unsigned int foo(unsigned int x)
{
  if (x < 5)
    x = 4;
  else
    x = 8;
  return x;
}

int main(void)
{
  if (foo (8) != 8)
    return 1;
  return 0;
}