// expect: 0
package main;
typedef long __attribute__((vector_size (16 * sizeof (long)))) v16di;
v16di g0, g1;
__attribute__((noipa)) static void
dirty_stack (void)
{
  volatile char buf[1024];
  for (unsigned i = 0; i < sizeof (buf); i++)
    buf[i] = 0xa5;
}
__attribute__((noipa)) static v16di
f (v16di p)
{
  v16di old = p;
  p = g0;
  g1 = p;
  return old + p;
}
int
main (void)
{
  v16di a, r;
  for (int i = 0; i < 16; i++)
    {
      a[i] = i + 1;
      g0[i] = 100;
    }
  dirty_stack ();
  r = f (a);
  for (int i = 0; i < 16; i++)
    if (r[i] != i + 101 || g1[i] != 100)
      __builtin_abort ();
  return 0;
}
