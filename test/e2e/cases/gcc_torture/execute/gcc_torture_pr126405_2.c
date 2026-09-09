// expect: 0
package main;
typedef long __attribute__((vector_size (16 * sizeof (long)))) v16di;
typedef int __attribute__((vector_size (8 * sizeof (int)))) v8si;
long g2, g12;
v16di g18;
v8si g3;
void *g27;
__attribute__((noipa)) static void
dirty_stack (void)
{
  volatile char buf[1024];
  for (unsigned i = 0; i < sizeof (buf); i++)
    buf[i] = 0xa5;
}
void
f31 (void)
{
lbl_br1:
  g18 = ~g18;
  g3 = ~g3;
  if (g2)
    goto lbl_br1;
lbl_b5:
  switch (g12)
    case 4:
    case 0:
      goto lbl_sw8;
  __builtin_abort ();
lbl_sw8:
  if (g27)
    goto lbl_b5;
  g18 = ~g18;
}
int
main (void)
{
  dirty_stack ();
  f31 ();
  for (int i = 0; i < 16; i++)
    if (g18[i] != 0)
      __builtin_abort ();
  return 0;
}
