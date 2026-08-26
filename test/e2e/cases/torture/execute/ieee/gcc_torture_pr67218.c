// expect: 0
package main;

extern void abort (void);

double
foo (unsigned int x)
{
  return (double) (float) (x | 0xffff0000);
}

int
main ()
{
  if (foo (1) != 0x1.fffep31)
    abort ();
  return 0;
}
