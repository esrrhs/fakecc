// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000412-2.c
package main;

int f(int a,int *y)
{
  int x = a;

  if (a==0)
    return *y;

  return f(a-1,&x);
}

int main(int argc,char **argv)
{
  if (f (100, (int *) 0) != 1)
    return 1;
  return 0;
}