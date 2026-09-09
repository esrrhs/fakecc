/* PR c/42544 */

// expect: 0
package main;

extern void abort(void);

int
main (void)
{
  signed short s = -1;
  if (sizeof (long long) == sizeof (unsigned int))
    return 0;
  if ((unsigned int) s >= 0x100000000ULL)
    abort ();
  return 0;
}
