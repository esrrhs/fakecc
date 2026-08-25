/* PR sanitizer/81281 */

// expect: 0
package main;

void
foo (unsigned p, unsigned a, unsigned b)
{
  unsigned q = p + 7;
  if (a - (1U + 2147483647U) >= 2)
    __builtin_unreachable ();
  int d = p + b;
  int c = p + a;
  if (c - d != 2147483647)
    __builtin_abort ();
}

void
bar (unsigned p, unsigned a)
{
  unsigned q = p + 7;
  if (a - (1U + 2147483647U) >= 2)
    __builtin_unreachable ();
  int c = p;
  int d = p + a;
  if (c - d != -2147483647 - 1)
    __builtin_abort ();
}

int
main (void)
{
  foo (-1U, 1U + 2147483647U, 1U);
  bar (-1U, 1U + 2147483647U);
  return 0;
}
