// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20021118-3.c
package main;

int
foo (int x)
{
  if (x == -2 || -x - 100 >= 0)
    return 1;
  return 0;
}
           
int
main ()
{
  foo (-3);
  foo (-99);
  return 0;
}