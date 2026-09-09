// expect: 0
package main;

/* PR target/85582 */

extern void abort (void);

typedef __int128 S;
typedef unsigned __int128 U;

S
f1 (S x, int y)
{
  x = x << (y & 5);
  x += y;
  return x;
}

S
f2 (S x, int y)
{
  x = x >> (y & 5);
  x += y;
  return x;
}

U
f3 (U x, int y)
{
  x = x >> (y & 5);
  x += y;
  return x;
}

int
main (void)
{
  S a = (S) 1 << (sizeof (S) * 8 - 7);
  S b = f1 (a, 12);
  if (b != ((S) 1 << (sizeof (S) * 8 - 3)) + 12)
    abort ();
  S c = (U) 1 << (sizeof (S) * 8 - 1);
  S d = f2 (c, 12);
  if ((U) d != ((U) 0x1f << (sizeof (S) * 8 - 5)) + 12)
    abort ();
  U e = (U) 1 << (sizeof (U) * 8 - 1);
  U f = f3 (c, 12);
  if (f != ((U) 1 << (sizeof (U) * 8 - 5)) + 12)
    abort ();
  return 0;
}
