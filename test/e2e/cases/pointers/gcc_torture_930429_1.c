// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930429-1.c
package main;

char *
f (char *p)
{
  short x = *p++ << 16;
  return p;
}

int
main (void)
{
  char *p = "";
  if (f (p) != p + 1)
    return 1;
  return 0;
}