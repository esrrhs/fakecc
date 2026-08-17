// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950714-1.c
package main;

int array[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

int
main (void)
{
  int i, j;
  int *p;

  for (i = 0; i < 10; i++)
    for (p = &array[0]; p != &array[9]; p++)
      if (*p == i)
	goto label;

 label:
  if (i != 1)
    return 1;
  return 0;
}