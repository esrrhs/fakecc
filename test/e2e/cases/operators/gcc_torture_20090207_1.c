// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20090207-1.c
package main;

int foo(int i)
{
  int a[32];
  a[1] = 3;
  a[0] = 1;
  a[i] = 2;
  return a[0];
}

int main()
{
  if (foo (0) != 2
      || foo (1) != 1)
    return 1;
  return 0;
}