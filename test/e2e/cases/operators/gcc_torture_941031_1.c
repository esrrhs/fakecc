// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/941031-1.c
package main;

typedef long mpt;

int
f (mpt us, mpt vs)
{
  long aus;
  long avs;

  aus = us >= 0 ? us : -us;
  avs = vs >= 0 ? vs : -vs;

  if (aus < avs)
    {
      long t = aus;
      aus = avs;
      avs = aus;
    }

  return avs;
}

int
main (void)
{
  if (f ((mpt) 3, (mpt) 17) != 17)
    return 1;
  return 0;
}