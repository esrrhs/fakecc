// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/961004-1.c
package main;

int k = 0;

int
main(void)
{
  int i;
  int j;

  for (i = 0; i < 2; i++)
    {
      if (k)
	{
	  if (j != 2)
	    return 1;
	}
      else
	{
	  j = 2;
	  k++;
	}
    }
  return 0;
}