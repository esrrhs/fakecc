// expect: 0
package main;
void abort (void);
void exit (int);
double dnan = 1.0/0.0 - 1.0/0.0;
double x = 1.0;
void leave ()
{
  exit (0);
}
int
main (void)
{
  if (dnan == dnan)
    abort ();
  if (dnan != x)
    x = 1.0;
  else
    abort ();
  if (dnan < x)
    abort ();
  if (dnan > x)
    abort ();
  if (dnan <= x)
    abort ();
  if (dnan >= x)
    abort ();
  if (dnan == x)
    abort ();
  exit (0);
}
