// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020219-1.c
package main;

/* PR c/4308
   This testcase failed because 0x8000000000000000 >> 0
   was incorrectly folded into 0xffffffff00000000.  */

long long foo (void)
{
  long long C = 1ULL << 63, X;
  int Y = 32;
  X = C >> (Y & 31);
  return X;
}

int main (void)
{
  if (foo () != 1ULL << 63)
    return 1;
  return 0;
}