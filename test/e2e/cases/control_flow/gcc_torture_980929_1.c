// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980929-1.c
package main;

int f(int i)
{
  if (i != 1000)
    return 1;

    return 0;}

int main()
{
  int n=1000;
  int i;

  f(n);
  for(i=0; i<1; ++i) {
    f(n);
    n=666;
    &n;
  }

  return 0;
}