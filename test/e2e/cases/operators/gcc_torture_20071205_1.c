// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20071205-1.c
package main;

/* PR middle-end/34337 */

int
foo (int x)
{
  return ((x << 8) & 65535) | 255;
}

int
main (void)
{
  if (foo (0x32) != 0x32ff || foo (0x174) != 0x74ff)
    return 1;
  return 0;
}