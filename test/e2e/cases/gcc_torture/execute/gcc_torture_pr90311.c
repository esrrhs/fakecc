// expect: 0
package main;

/* PR rtl-optimization/90311 */

extern void abort (void);

int a, b;

int
main (void)
{
  unsigned long long x;
  unsigned int c;
  __builtin_add_overflow ((unsigned char) a, b, &c);
  b -= c < (unsigned char) a;
  x = b;
  if (x)
    abort ();
  return 0;
}
