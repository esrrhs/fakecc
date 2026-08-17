// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20070212-2.c
package main;

int f(int k, int i1, int j1)
{
  int *f1;
  if(k)
   f1 = &i1;
  else
   f1 = &j1;
  i1 = 0;
  return *f1;
}

int main()
{
  if (f(1, 1, 2) != 0)
    return 1;
  return 0;
}