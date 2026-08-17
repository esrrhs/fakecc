// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20050410-1.c
package main;

int s = 200;
int 
foo (void)
{
  return (signed char) (s - 100) - 5;
}
int
main (void)
{
  if (foo () != 95)
    return 1;
  return 0;
}