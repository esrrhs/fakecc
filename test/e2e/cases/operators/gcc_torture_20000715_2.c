// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000715-2.c
package main;

unsigned int foo(unsigned int a)
{
  return ((unsigned char)(a + 1)) * 4;
}

int main(void)
{
  if (foo((unsigned char)~0))
    return 1;
  return 0;
}