/* PR rtl-optimization/70222 */

// expect: 0
package main;

int a = 1;
unsigned int b = 2;
int c = 0;
int d = 0;

void
foo (void)
{
  int e = ((-(c >= c)) < b) > ((int) (-1ULL >> ((a / a) * 15)));
  d = -e;
}

__attribute__((noinline)) void
bar (int x)
{
  if (x != -1)
    __builtin_abort ();
}

int
main (void)
{
  foo ();
  bar (d);
  return 0;
}
