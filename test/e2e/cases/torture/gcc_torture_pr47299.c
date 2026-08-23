/* PR rtl-optimization/47299 */

// expect: 0
package main;

extern void abort(void);

__attribute__ ((noinline)) unsigned short
foo (unsigned char x)
{
  return x * 255;
}

int
main (void)
{
  if (foo (0x40) != 0x3fc0)
    abort ();
  return 0;
}
