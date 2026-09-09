/* PR target/69403.  */

// expect: 0
package main;

int a, b, c;

__attribute__ ((__noinline__)) int
fn1 (void)
{
  if ((b | (a != (a & c))) == 1)
    __builtin_abort ();
  return 0;
}

int
main (void)
{
  a = 5;
  c = 1;
  b = 6;
  return fn1 ();
}
