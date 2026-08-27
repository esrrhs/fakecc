// expect: 0
package main;
void abort (void);
void exit (int);
long double dnan = 1.0l/0.0l - 1.0l/0.0l;
long double x = 1.0l;
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
