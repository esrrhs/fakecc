/* PR rtl-optimization/89195 */

// expect: 0
package main;

struct S { unsigned i : 24; };

volatile unsigned char x;

__attribute__((noipa)) int
foo (struct S d) 
{
  return d.i & x;
}

int
main (void)
{
  struct S d = { 0x123456 };
  x = 0x75;
  if (foo (d) != (0x56 & 0x75))
    __builtin_abort ();
  return 0;
}
