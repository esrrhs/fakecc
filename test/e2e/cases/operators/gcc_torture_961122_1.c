// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/961122-1.c
package main;

long long acc;

void
addhi (short a)
{
  acc += (long long) a << 32;
}

void
subhi (short a)
{
  acc -= (long long) a << 32;
}

int
main (void)
{
  acc = 0xffff00000000ll;
  addhi (1);
  if (acc != 0x1000000000000ll)
    return 1;
  subhi (1);
  if (acc != 0xffff00000000ll)
    return 1;
  return 0;
}