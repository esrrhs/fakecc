// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921202-2.c
package main;

int
f(long long x)
{
  x >>= 8;
  return x & 0xff;
}

int
main(void)
{
  if (f(0x0123456789ABCDEFLL) != 0xCD)
    return 1;
  return 0;
}