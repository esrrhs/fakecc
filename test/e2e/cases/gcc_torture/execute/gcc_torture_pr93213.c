// expect: 0
package main;

/* PR tree-optimization/93213 */

extern void abort (void);
extern void *memcpy (void *, const void *, unsigned long);
extern void *memmove (void *, const void *, unsigned long);

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned __int128 u128;

static inline u128
foo (u16 u16_1, u32 u32_1, u128 u128_1)
{
  u128 u128_0 = 0;
  u128_1 -= __builtin_mul_overflow (u32_1, u16_1, &u32_1);
  memmove (&u16_1, &u128_0, 2);
  memmove (&u16_1, &u128_1, 1);
  return u16_1;
}

void
bar (void)
{
  char a[] = { 1, 2 };
  const char b[] = { 0, 0 };
  const char c[] = { 2 };
  memcpy (a, b, 2);
  memcpy (a, c, 1);

  char *p = a;
  if (p[0] != 2 || p[1] != 0)
    abort ();
}

int
main (void)
{
  u16 x = foo (-1, -1, 0);
  if (x != 0xff)
    abort ();

  bar ();
  return 0;
}
