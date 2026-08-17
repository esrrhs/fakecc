// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040309-1.c
package main;

int foo(unsigned short x)
{
  unsigned short y;
  y = x > 32767 ? x - 32768 : 0;
  return y;
}

int main()
{
  if (foo (0) != 0)
    return 1;
  if (foo (32767) != 0)
    return 1;
  if (foo (32768) != 0)
    return 1;
  if (foo (32769) != 1)
    return 1;
  if (foo (65535) != 32767)
    return 1;
  return 0;
}