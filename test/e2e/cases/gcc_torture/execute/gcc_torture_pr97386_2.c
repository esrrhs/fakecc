// expect: 0
package main;

/* PR rtl-optimization/97386 */

extern void abort (void);

unsigned
foo (int x)
{
  unsigned long long a = (0x800000000000ccccULL << x) | (0x800000000000ccccULL >> (64 - x));
  unsigned int b = a;
  return (b << 24) | (b >> 8);
}

int
main ()
{
  if (foo (1) != 0x99000199U)
    abort ();
  return 0;
}
