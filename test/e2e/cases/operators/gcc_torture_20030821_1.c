// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030821-1.c
package main;

int
foo (int x)
{
  if ((int) (x & 0x80ffffff) != (int) (0x8000fffe))
    return 1;

  return 0;
}

int
main ()
{
  return foo (0x8000fffe);
}