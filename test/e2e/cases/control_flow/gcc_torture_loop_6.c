// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-6.c
package main;

int
main(void)
{
  char c;
  char d;
  int nbits;
  c = -1;
  for (nbits = 1 ; nbits < 100; nbits++) {
    d = (1 << nbits) - 1;
    if (d == c)
      break;
  }
  if (nbits == 100)
    return 1;
  return 0;
}