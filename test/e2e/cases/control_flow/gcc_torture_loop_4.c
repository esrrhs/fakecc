// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-4.c
package main;

int
f()
{
  int j = 1;
  long i;
  for (i = -0x70000000L; i < 0x60000000L; i += 0x10000000L) j <<= 1;
  return j;
}

int
main ()
{
  if (f () != 8192)
    return 1;
  return 0;
}