/* PR rtl-optimization/68249 */

// expect: 0
package main;

int a, b, c, g, k, l, m, n;
char h;

void
fn1 (void)
{
  for (; k; k++)
    {
      m = (b || c < 0 || c > 1) ? 0 : c;
      g = l = (n || m < 0 || (m > 1) > 1 >> m) ? 0 : 1 << m;
    }
  l = b + 1;
  for (; b < 1; b++)
    h = a + 1;
}

int
main (void)
{
  char j; 
  for (; a < 1; a++)
    {
      fn1 ();
      if (h)
	j = h;
      if (j > c)
	g = 0;
    }

  if (h != 1) 
    __builtin_abort (); 

  return 0;
}
