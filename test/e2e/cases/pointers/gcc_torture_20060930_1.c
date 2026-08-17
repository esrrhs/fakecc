// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20060930-1.c
package main;

/* PR rtl-optimization/28096 */
/* Origin: Jan Stein <jan@gatespacetelematics.com> */

int bar (int, int) ;
int bar (int a, int b)
{
  if (b != 1)
    return 1;
}

void foo(int, int) ;
void foo (int e, int n)
{
  int i, bb2, bb5;

  if (e > 0)
    e = -e;

  for (i = 0; i < n; i++)
    {
      if (e >= 0)
	{
	  bb2 = 0;
	  bb5 = 0;
	}
      else
	{
	  bb5 = -e;
	  bb2 = bb5;
	}

      bar (bb5, bb2);
    }
}

int main(void)
{
  foo (1, 1);
  return 0;
}