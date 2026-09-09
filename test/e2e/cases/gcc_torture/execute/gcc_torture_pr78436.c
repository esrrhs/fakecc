/* PR tree-optimization/78436 */

// expect: 0
package main;

struct S
{
  long int a : 24;
  signed char b : 8;
} s;

__attribute__((noinline)) void
foo (void)
{
  s.b = 0;
  s.a = -1193165L;
}

int
main (void)
{
  foo ();
  if (s.b != 0)
    __builtin_abort ();
  return 0;
}
