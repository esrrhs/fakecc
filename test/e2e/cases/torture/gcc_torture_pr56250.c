/* PR tree-optimization/56250 */

// expect: 0
package main;

extern void abort(void);

int
main (void)
{
  unsigned int x = 2;
  unsigned int y = (0U - x / 2) / 2;
  if (-1U / x != y)
    abort ();
  return 0;
}
