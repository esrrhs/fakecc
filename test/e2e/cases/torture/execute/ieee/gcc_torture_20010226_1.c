// expect: 0
package main;

void abort (void);

long double dfrom = 1.1L;
long double m1;
long double m2;
unsigned long mant_long;

int main()
{
  m1 = dfrom / 2.0L;
  m2 = m1 * 4294967296.0L;
  mant_long = ((unsigned long) m2) & 0xffffffff;

  if (mant_long == 0x8ccccccc)
    return 0;
  else
    abort();
}
