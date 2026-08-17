// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20011217-1.c
package main;

int
main()
{
  double x = 1.0;
  double y = 2.0;

  if ((y > x--) != 1)
    return 1;
  return 0;
}