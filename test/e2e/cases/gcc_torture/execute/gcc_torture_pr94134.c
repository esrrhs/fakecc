/* PR target/94134 */

// expect: 0
package main;

static volatile int a = 0;
static volatile int b = 1;

int
main (void)
{
  a++;
  b++;
  if (a != 1 || b != 2)
    __builtin_abort ();
  return 0;
}
