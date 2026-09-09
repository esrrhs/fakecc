/* PR middle-end/38422 */

// expect: 0
package main;

extern void abort(void);

struct S
{
  int s : 30;
} s;

void
foo (void)
{
  s.s *= 2;
}

int
main (void)
{
  s.s = 24;
  foo ();
  if (s.s != 48)
    abort ();
  return 0;
}
