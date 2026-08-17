// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20100805-1.c
package main;

unsigned int foo (unsigned int a, unsigned int b)
{
  unsigned i;
  a = a & 1;
  for (i = 0; i < b; ++i)
    a = a << 1 | a >> (sizeof (unsigned int) * 8 - 1);
  return a;
}

int main()
{
  if (foo (1, sizeof (unsigned int) * 8 + 1) != 2)
    return 1;
  return 0;
}