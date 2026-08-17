// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990527-1.c
package main;

int sum;

void
g (int i)
{
  sum += i;
}

void
f(int j)
{
  int i;

  for (i = 0; i < 9; i++)
    {
      j++;
      g (j);
      j = 9;
    }
}

int
main ()
{
  f (0);
  if (sum != 81)
    return 1;
  return 0;
}