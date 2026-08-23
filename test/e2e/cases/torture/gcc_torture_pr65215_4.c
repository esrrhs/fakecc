/* PR tree-optimization/65215 */

// expect: 0
package main;

struct S { unsigned long long l1 : 48; };

static inline unsigned int
foo (unsigned int x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

__attribute__((noinline)) unsigned int
bar (struct S *x)
{
  return foo (x->l1);
}

int
main (void)
{
  struct S s;
  s.l1 = foo (0xdeadbeefU) | (0xfeedULL << 32);
  if (bar (&s) != 0xdeadbeefU)
    __builtin_abort ();
  return 0;
}
