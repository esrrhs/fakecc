// expect: 0
// link: -lm
package main;

extern double pow(double, double);
void abort (void);
void exit (int);

int
main(void)
{
  volatile double a;
  double c;
  a = 32.0;
  c = pow(a, 1.0/3.0);
  if (c + 0.1 > 3.174802
      && c - 0.1 < 3.174802)
    exit (0);
  else
    abort ();
}
