/* PR tree-optimization/56051 */

// expect: 0
package main;

extern void abort(void);

int
main (void)
{
  unsigned char x1[1] = { 0 };
  unsigned int s1 = 8;
  int a1 = x1[0] < (unsigned char) (1 << s1);
  unsigned char y1 = (unsigned char) (1 << s1);
  int b1 = x1[0] < y1;
  if (a1 != b1)
    abort ();
  return 0;
}
