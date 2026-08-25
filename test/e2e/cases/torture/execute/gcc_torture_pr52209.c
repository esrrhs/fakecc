/* PR middle-end/52209 */

// expect: 0
package main;

extern void abort(void);

struct S0 { int f2 : 1; } c;
int b;

int
main (void)
{
  b = -1 ^ c.f2;
  if (b != -1)
    abort ();
  return 0;
}
