// expect: 0
package main;

/* PR target/98853 */

extern void abort (void);
extern void *memcpy (void *, const void *, unsigned long);

unsigned long long
foo (unsigned x, unsigned long long y, unsigned long long z)
{
  memcpy (2 + (char *) &x, 2 + (char *) &y, 2);
  return x + z;
}

int
main ()
{
  if (foo (0x44444444U, 0x1111111111111111ULL, 0x2222222222222222ULL)
      != 0x2222222233336666ULL)
    abort ();
  return 0;
}
