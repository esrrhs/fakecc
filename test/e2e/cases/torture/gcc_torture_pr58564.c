/* PR middle-end/58564 */

// expect: 0
package main;

extern void abort(void);

int a, b;
short *c, **d = &c;

int
main (void)
{
  b = (0, 0 > ((&c == d) & (1 && (a ^ 1)))) | 0U;
  if (b != 0)
    abort ();
  return 0;
}
