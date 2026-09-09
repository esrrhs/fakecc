// expect: 0
package main;

/* PR c/58943 */

extern void abort (void);

unsigned int x[1] = { 2 };

unsigned int
foo (void)
{
  x[0] |= 128;
  return 1;
}

int
main (void)
{
  x[0] |= foo ();
  if (x[0] != 131)
    abort ();
  return 0;
}
