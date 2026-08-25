/* PR tree-optimization/49073 */

// expect: 0
package main;

extern void abort(void);

int a[7] = { 1, 2, 3, 4, 5, 6, 7 }, c;

int
main (void)
{
  int d = 1, i = 1;
  int f = 0;
  do
    {
      d = a[i];
      if (f && d == 4)
	{
	  ++c;
	  break;
	}
      i++;
      f = (d == 3);
    }
  while (d < 7);
  if (c != 1)
    abort ();
  return 0;
}
