// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000523-1.c
package main;

int
main (void)
{
  long long   x;
  int         n;

  if (sizeof (long long) < 8)
    return 0;
  
  n = 9;
  x = (((long long) n) << 55) / 0xff; 

  if (x == 0)
    return 1;

  x = (((long long) 9) << 55) / 0xff;

  if (x == 0)
    return 1;

  return 0;
}