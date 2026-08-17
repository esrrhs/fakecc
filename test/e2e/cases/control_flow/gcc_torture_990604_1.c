// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990604-1.c
package main;

int b;
void f ()
{
  int i = 0;
  if (b == 0)
    do {
      b = i;
      i++;
    } while (i < 10);
}

int main ()
{
  f ();
  if (b != 9)
    return 1;
  return 0;
}