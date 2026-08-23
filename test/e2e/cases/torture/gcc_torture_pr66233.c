/* PR tree-optimization/66233 */

// expect: 0
package main;

unsigned int v[8];

__attribute__((noinline)) void
foo (void)
{
  int i;
  for (i = 0; i < 8; i++)
    v[i] = (float) i;
}

int
main (void)
{
  unsigned int i;
  foo ();
  for (i = 0; i < 8; i++)
    if (v[i] != i)
      __builtin_abort ();
  return 0;
}
