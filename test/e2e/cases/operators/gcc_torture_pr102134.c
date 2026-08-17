// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr102134.c
package main;

/* PR tree-optimization/102134 */

typedef unsigned long long u64;

u64 g;

void
foo (u64 a, u64 b, u64 c, u64 *r)
{
  b *= b;
  u64 x = a && ((b >> (c & 63)) | ((b << (c & 63)) & g));
  *r = x + a;
}

int
main ()
{
  u64 x;
  foo (1, 3000, 0, &x);
  if (x != 2)
    return 1;
  return 0;
}