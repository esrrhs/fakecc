// expect: 0
package main;

/* PR target/89434 */

extern void abort (void);

long g = 0;

static inline unsigned long long
foo (unsigned long long u)
{
  unsigned x;
  __builtin_mul_overflow (-1, g, &x);
  u |= (unsigned) u < (unsigned short) x;
  return x - u;
}

int
main (void)
{
  unsigned long long x = foo (0x222222222ULL);
  if (x != 0xfffffffddddddddeULL)
    abort ();
  return 0;
}
