// expect: 0
package main;

/* PR target/98681 */

extern void abort (void);

int
foo (int x)
{
  if (x > 32)
    return (x << -64) & 255;
  else
    return x;
}

int
main ()
{
  if (foo (32) != 32 || foo (-150) != -150)
    abort ();
  return 0;
}
