// expect: 0
package main;
typedef long __attribute__((vector_size (16 * sizeof (long)))) v16di;
v16di g0 = { 1 }, g1 = { 5 }, g2 = { 3 }, g3 = { 7 };
volatile int p, q;
int
main (void)
{
  v16di b = g0;
  v16di a;
  if (p)
    a = b;
  else
    a = g1;
  g1 = a;
  b = g2;
  v16di c;
  if (q)
    c = b;
  else
    c = g3;
  g3 = c;
  if (g1[0] != 5 || g1[1] != 0 || g3[0] != 7 || g3[1] != 0)
    __builtin_abort ();
  return 0;
}
