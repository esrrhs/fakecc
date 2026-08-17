// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030120-2.c
package main;

/* PR 8848 */

int foo(int status)
{
  int s = 0;
  if (status == 1) s=1;
  if (status == 3) s=3;
  if (status == 4) s=4;
  return s;
}

int main()
{
  if (foo (3) != 3)
    return 1;
  return 0;
}