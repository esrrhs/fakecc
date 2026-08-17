// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr109778.c
package main;

/* PR tree-optimization/109778 */

int a, b, c, d, *e = &c;

static inline unsigned
foo (unsigned char x)
{
  x = 1 | x << 1;
  x = x >> 4 | x << 4;
  return x;
}

static inline void
bar (unsigned x)
{
  *e = 8 > foo (x + 86) - 86;
}

int
main ()
{
  d = a && b;
  bar (d + 4);
  if (c != 1)
    return 1;
}