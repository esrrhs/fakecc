/* PR rtl-optimization/65401 */

// expect: 0
package main;

struct S { unsigned short s[64]; };

__attribute__((noinline)) void
foo (struct S *x)
{
  unsigned int i;
  unsigned char *s;

  s = (unsigned char *) x->s;
  for (i = 0; i < 64; i++)
    x->s[i] = s[i * 2] | (s[i * 2 + 1] << 8);
}

__attribute__((noinline)) void
bar (struct S *x)
{
  unsigned int i;
  unsigned char *s;

  s = (unsigned char *) x->s;
  for (i = 0; i < 64; i++)
    x->s[i] = (s[i * 2] << 8) | s[i * 2 + 1];
}

int
main (void)
{
  unsigned int i;
  struct S s;
  for (i = 0; i < 64; i++)
    s.s[i] = i + ((64 - i) << 8);
  foo (&s);
  for (i = 0; i < 64; i++)
    if (s.s[i] != i + ((64 - i) << 8))
      __builtin_abort ();
  for (i = 0; i < 64; i++)
    s.s[i] = i + ((64 - i) << 8);
  bar (&s);
  for (i = 0; i < 64; i++)
    if (s.s[i] != (64 - i) + (i << 8))
      __builtin_abort ();
  return 0;
}
