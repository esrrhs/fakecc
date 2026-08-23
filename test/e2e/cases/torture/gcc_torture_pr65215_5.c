/* PR tree-optimization/65215 */

// expect: 0
package main;

__attribute__((noinline)) unsigned int
foo (unsigned char *p)
{
  return ((unsigned int) p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

__attribute__((noinline)) unsigned int
bar (unsigned char *p)
{
  return ((unsigned int) p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}

struct S { unsigned int a; unsigned char b[5]; };

int
main (void)
{
  struct S s = { 1, { 2, 3, 4, 5, 6 } };
  if (foo (&s.b[1]) != 0x03040506U
      || bar (&s.b[1]) != 0x06050403U)
    __builtin_abort ();
  return 0;
}
