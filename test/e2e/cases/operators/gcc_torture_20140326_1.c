// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20140326-1.c
package main;

int a;

int
main (void)
{
  char e[2] = { 0, 0 }, f = 0;
  if (a == 131072)
    f = e[a];
  return f;
}