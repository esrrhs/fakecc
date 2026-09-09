/* PR middle-end/94724 */

// expect: 0
package main;

short a, b;

int
main (void)
{
  (0, (0, (a = 0 >= 0, b))) != 53601;
  if (a != 1)
    __builtin_abort ();
  return 0;
}
