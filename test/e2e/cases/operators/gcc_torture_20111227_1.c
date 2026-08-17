// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20111227-1.c
package main;

/* PR rtl-optimization/51667 */
/* Testcase by Uros Bizjak <ubizjak@gmail.com> */

int 
bar (int a)
{
  if (a != -1)
    return 1;

    return 0;}

void 
foo (short *a, int t)
{
  short r = *a;

  if (t)
    bar ((unsigned short) r);
  else
    bar ((signed short) r);
}

short v = -1;

int main(void)
{
  foo (&v, 0);
  return 0;
}