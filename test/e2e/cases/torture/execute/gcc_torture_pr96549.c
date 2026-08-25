/* PR c/96549 */

// expect: 0
package main;

long c = -1L;
long b = 0L;

int
main (void)
{
  if (3L > (short) ((c ^= (b = 1L)) * 3L))
    return 0;
  __builtin_abort ();
}
