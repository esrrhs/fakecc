// expect: 0
package main;
void abort (void);
void exit (int);
float fnan = 1.0f/0.0f - 1.0f/0.0f;
float x = 1.0f;
void leave ()
{
  exit (0);
}
int
main (void)
{
  if (fnan == fnan)
    abort ();
  if (fnan != x)
    x = 1.0;
  else
    abort ();
  if (fnan < x)
    abort ();
  if (fnan > x)
    abort ();
  if (fnan <= x)
    abort ();
  if (fnan >= x)
    abort ();
  if (fnan == x)
    abort ();
  exit (0);
}
