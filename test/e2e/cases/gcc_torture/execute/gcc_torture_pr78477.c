/* PR rtl-optimization/78477 */

// expect: 0
package main;

unsigned a;
unsigned short b;

unsigned
foo (unsigned x)
{
  b = x;
  a >>= (b & 1);
  b = 1 | (b << 5);
  b >>= 15;
  x = (unsigned char) b > ((2 - (unsigned char) b) & 1);
  b = 0;
  return x;
}

int
main (void)
{
  unsigned x = foo (12345);
  if (x != 0)
    __builtin_abort ();
  return 0;
}
