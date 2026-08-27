// expect: 0
package main;

const double dnan = 1.0/0.0 - 1.0/0.0;
double x = 1.0;

extern void abort (void);
extern void exit (int);

void link_error (void)
{
  abort ();
}

int
main (void)
{
  /* NaN is an IEEE unordered operand.  All these test should be false.  */
  if (dnan == dnan)
    link_error ();
  if (dnan != x)
    x = 1.0;
  else
    link_error ();

  if (dnan == x)
    link_error ();
  exit (0);
}
