// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010711-1.c
package main;

void foo (int *a) {}

int main ()
{
  int a;
  if (&a == 0)
    return 1;
  else
    {
      foo (&a);
      return 0;
    }
}