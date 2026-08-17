// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020225-2.c
package main;

static int 
test(int x)
{
  union 
    {
      int i;
      double d;
  } a;
  a.d = 0;
  a.i = 1;
  return x >> a.i;
}

int main(void)
{
  if (test (5) != 2)
    return 1;
  return 0;
}