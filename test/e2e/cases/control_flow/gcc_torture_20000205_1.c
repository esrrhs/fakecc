// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000205-1.c
package main;

static int f (int a)
{
  if (a == 0)
    return 0;
  do
    if (a & 128)
      return 1;
  while (f (0));
  return 0;
}

int main(void)
{
  if (f (~128))
    return 1;
  return 0;
}