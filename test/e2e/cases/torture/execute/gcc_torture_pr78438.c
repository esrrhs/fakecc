// expect: 0
package main;

/* PR target/78438 */

extern void abort (void);

char a = 0;
int b = 197412621;

void
foo (void)
{
  a = 0 > (short) (b >> 11);
}

int
main (void)
{
  if (sizeof (short) != 2 || sizeof (int) < 4)
    return 0;
  foo ();
  if (a != 0)
    abort ();
  return 0;
}
