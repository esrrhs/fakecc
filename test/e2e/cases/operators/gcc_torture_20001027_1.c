// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001027-1.c
package main;

int x,*p=&x;

int main()
{
  int i=0;
  x=1;
  p[i]=2;
  if (x != 2)
    return 1;
  return 0;
}