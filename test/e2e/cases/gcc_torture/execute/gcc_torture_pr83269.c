/* PR tree-optimization/83269 */

// expect: 0
package main;

int
main (void)
{
  volatile unsigned char a = 1;
  long long b = 0x80000000L;
  int c = -((int)(-b) - (-0x7fffffff * a));
  if (c != 1)
    __builtin_abort ();
  return 0;
}
