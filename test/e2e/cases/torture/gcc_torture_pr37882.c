/* PR middle-end/37882 */

// expect: 0
package main;

struct S
{
  unsigned char b : 3;
} s;

int
main (void)
{
  s.b = 4;
  if (s.b > 0 && s.b < 4)
    __builtin_abort ();
  return 0;
}
