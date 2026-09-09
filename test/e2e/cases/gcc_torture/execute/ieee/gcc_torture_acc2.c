// expect: 0
package main;

/* Tail call optimizations would reverse the order of multiplications
   in func().  */

void abort (void);
void exit (int);

double func (const double *array)
{
  double d = *array;
  if (d == 1.0)
    return d;
  else
    return d * func (array + 1);
}

int main ()
{
  double values[] = { 1.7976931348623157e+308, 2.0, 0.5, 1.0 };
  if (func (values) != 1.7976931348623157e+308)
    abort ();
  exit (0);
}
