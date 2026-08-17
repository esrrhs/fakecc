// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20031211-1.c
package main;

struct a { unsigned int bitfield : 1; };

unsigned int x;

int
main(void)
{
  struct a a = {0};
  x = 0xbeef;
  a.bitfield |= x;
  if (a.bitfield != 1)
    return 1;
  return 0;
}