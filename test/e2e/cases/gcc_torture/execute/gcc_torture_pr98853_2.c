// expect: 0
package main;

/* PR target/98853 */

extern void abort (void);

unsigned long long
foo (unsigned long long x, unsigned int y)
{
  return ((unsigned) x & 0xfffe0000U) | (y & 0x1ffff);
}

int
main ()
{
  if (foo (0xdeadbeefcaf2babeULL, 0xdeaffeedU) != 0x00000000caf3feedULL)
    abort ();
  return 0;
}
