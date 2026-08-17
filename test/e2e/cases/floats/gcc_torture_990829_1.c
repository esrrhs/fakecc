// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990829-1.c
package main;

double test (const double le, const double ri)
{
	double val = ( ri - le ) / ( ri * ( le + 1.0 ) );
	return val;
}

int main ()
{
        double retval;

	retval = test(1.0,2.0);
        if (retval < 0.24 || retval > 0.26)
	  return 1;
	return 0;
}