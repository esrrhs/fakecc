// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030117-1.c
package main;

int foo (int, int, int);
int bar (int, int, int);

int main (void)
{
  if (foo (5, 10, 21) != 12)
    return 1;

  if (bar (9, 12, 15) != 150)
    return 1;

  return 0;
}

int foo (int x, int y, int z)
{
  return (x + y + z) / 3;
}

int bar (int x, int y, int z)
{
  return foo (x * x, y * y, z * z);
}