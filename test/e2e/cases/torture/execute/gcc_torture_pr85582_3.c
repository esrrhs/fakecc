// expect: 0
package main;

/* PR target/85582 */

extern void abort (void);

typedef __int128 S;
typedef unsigned __int128 U;

U
f1 (U x, int y)
{
  return x << (y & -2);
}

S
f2 (S x, int y)
{
  return x >> (y & -2);
}

U
f3 (U x, int y)
{
  return x >> (y & -2);
}

int
main (void)
{
  U a = (U) 1 << (sizeof (U) * 8 - 7);
  if (f1 (a, 5) != ((U) 1 << (sizeof (S) * 8 - 3)))
    abort ();
  S b = (U) 0x101 << (sizeof (S) * 8 / 2 - 7);
  if (f1 (b, sizeof (S) * 8 / 2) != (U) 0x101 << (sizeof (S) * 8 - 7))
    abort ();
  if (f1 (b, sizeof (S) * 8 / 2 + 2) != (U) 0x101 << (sizeof (S) * 8 - 5))
    abort ();
  S c = (U) 1 << (sizeof (S) * 8 - 1);
  if ((U) f2 (c, 5) != ((U) 0x1f << (sizeof (S) * 8 - 5)))
    abort ();
  if ((U) f2 (c, sizeof (S) * 8 / 2) != ((U) -1 << (sizeof (S) * 8 / 2 - 1)))
    abort ();
  if ((U) f2 (c, sizeof (S) * 8 / 2 + 2) != ((U) -1 << (sizeof (S) * 8 / 2 - 3)))
    abort ();
  U d = (U) 1 << (sizeof (S) * 8 - 1);
  if (f3 (c, 5) != ((U) 0x1 << (sizeof (S) * 8 - 5)))
    abort ();
  if (f3 (c, sizeof (S) * 8 / 2) != ((U) 1 << (sizeof (S) * 8 / 2 - 1)))
    abort ();
  if (f3 (c, sizeof (S) * 8 / 2 + 2) != ((U) 1 << (sizeof (S) * 8 / 2 - 3)))
    abort ();
  return 0;
}
