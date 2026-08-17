// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr107131.c
package main;

/* PR target/107131 */

 unsigned long long
foo (unsigned char o)
{
  unsigned long long t1 = -(long long) (o == 0);
  unsigned long long t2 = -(long long) (t1 > 10439075533421201520ULL);
  unsigned long long t3 = -(long long) (t1 <= t2);
  return t3;
}

int
main ()
{
  if (foo (0) != -1ULL)
    return 1;
  return 0;
}