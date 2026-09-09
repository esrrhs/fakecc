// expect: 0
package main;

/* PR target/93494 */

extern void abort (void);

unsigned short a;

int
main (void)
{
  register unsigned long long y = 0;
  int x = __builtin_add_overflow (y, 0ULL, &a);
  if (x || a)
    abort ();
  return 0;
}
