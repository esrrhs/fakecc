// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/ifcvt-onecmpl-abs-1.c
package main;

int foo(int n)
{
  if (n < 0)
    n = ~n;

  return n;
}

int main(void)
{
  if (foo (-1) != 0)
    return 1;

  return 0;
}