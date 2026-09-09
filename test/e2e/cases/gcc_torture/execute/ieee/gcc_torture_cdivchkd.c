// expect: 0
package main;

/*
  Program to test complex divide for correct results on selected values.
  Checking known failure points.
*/

extern void abort (void);
extern void exit (int);

int ilogb (double x)
{
  if (x == 0.0) return -2147483647 - 1;
  if (x != x) return 2147483647;
  union { double d; unsigned long long u; } u;
  u.d = x;
  int exp = (int)((u.u >> 52) & 0x7ff);
  if (exp == 0x7ff) return 2147483647;
  if (exp == 0) {
    unsigned long long frac = u.u & 0xfffffffffffffULL;
    int shift = 0;
    while ((frac & 0x10000000000000ULL) == 0) {
      frac <<= 1;
      shift++;
    }
    return -1022 - shift;
  }
  return exp - 1023;
}

double fmax (double x, double y)
{
  if (x != x) return y;
  if (y != y) return x;
  return x > y ? x : y;
}

int match (double _Complex, double _Complex);

const double SMALL = 2.2250738585072014e-308;
const int MAXBIT = 53;
const int ERRLIM = 6;

/*
  Compare c (computed value) with z (expected value).
  Return 0 if within allowed range.  Return 1 if not.
*/
int match (double _Complex c, double _Complex z)
{
  double rz, iz, rc, ic;
  double rerr, ierr, rmax;
  int biterr;
  rz = __real__ z;
  iz = __imag__ z;
  rc = __real__ c;
  ic = __imag__ c;

  if (__builtin_fabs (rz) > SMALL)
    {
      rerr = __builtin_fabs (rz - rc) / __builtin_fabs (rz);
    }
  else if (__builtin_fabs (rz) == 0.0)
    {
      rerr = __builtin_fabs (rc);
    }
  else
    {
      rerr = __builtin_fabs (rz - rc) / SMALL;
    }

  if (__builtin_fabs (iz) > SMALL)
    {
      ierr = __builtin_fabs (iz - ic) / __builtin_fabs (iz);
    }
  else if (__builtin_fabs (iz) == 0.0)
    {
      ierr = __builtin_fabs (ic);
    }
  else
    {
      ierr = __builtin_fabs (iz - ic) / SMALL;
    }
  rmax = fmax(rerr, ierr);
  biterr = 0;
  if ( rmax != 0.0)      
    {
      biterr = ilogb (rmax) + MAXBIT + 1;
    }

  if (biterr >= ERRLIM)
    return 0;
  else
    return 1;
}


int main (int argc, char** argv)
{
  double _Complex a,b,c,z;
  double xr[4], xi[4], yr[4], yi[4], zr[4], zi[4];
  double cr, ci;
  int i;
  int ok = 1;
  xr[0] = -0x1.16e7fad79e45ep+651;
  xi[0] = -0x1.f7f75b94c6c6ap-860;
  yr[0] = -0x1.2f40d8ff7e55ep+245;
  yi[0] = -0x0.0000000004ebcp-1022;
  zr[0] = 0x1.d6e4b0e282869p+405;
  zi[0] = -0x1.e9095e311e706p-900;

  xr[1] = -0x1.21ff587f953d3p-310;
  xi[1] = -0x1.5a526dcc59960p+837;
  yr[1] = 0x1.b88b8b552eaadp+735;
  yi[1] = -0x1.873e2d6544d92p-327;
  zr[1] = 0x1.65734a88b2de0p-961;
  zi[1] =  -0x1.927e85b8b5770p+101;

  xr[2] = 0x1.4612e41aa8080p-846;
  xi[2] = -0x0.0000000613e07p-1022;
  yr[2] = 0x1.df9cd0d58caafp-820;
  yi[2] = -0x1.e47051a9036dbp-584;
  zr[2] = 0x1.9b194f3fffa32p-469;
  zi[2] = 0x1.58a00ab740a6bp-263;

  xr[3] = 0x1.cb27eece7c585p-355;
  xi[3] = 0x0.000000223b8a8p-1022;
  yr[3] = -0x1.74e7ed2b9189fp-22;
  yi[3] = 0x1.3d80439e9a119p-731;
  zr[3] = -0x1.3b35ed806ae5ap-333;
  zi[3] = -0x0.05e01bcbfd9f6p-1022;


  for (i = 0; i < 4; i++)
    {
      __real__ a = xr[i];
      __imag__ a = xi[i];
      __real__ b = yr[i];
      __imag__ b = yi[i];
      __real__ z = zr[i];
      __imag__ z = zi[i];
      c = a / b;
      cr = __real__ c;
      ci = __imag__ c;

      if (!match (c,z)){
	ok = 0;
      }
    }
  if (!ok)
    abort ();
  exit (0);
}
