// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991202-3.c
package main;

unsigned int f (unsigned int a)
{
  return a * 65536 / 8;
}

unsigned int g (unsigned int a)
{
  return a * 65536;
}

unsigned int h (unsigned int a)
{
  return a / 8;
}

int main ()
{
  if (f (65536) != h (g (65536)))
    return 1;
  return 0;
}