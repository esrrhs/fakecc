// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/981206-1.c
package main;

/* Verify unaligned address aliasing on Alpha EV[45].  */

static unsigned short x, y;

void foo()
{
  x = 0x345;
  y = 0x567;
}

int main()
{
  foo ();
  if (x != 0x345 || y != 0x567)
    return 1;
  return 0;
}