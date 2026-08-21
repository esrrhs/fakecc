// expect: 0
package main;
extern void abort (void);
int f1()
{
  return (int)2147483648.0f;
}
int f2()
{
  return (int)(float)(2147483647);
}
int main()
{
  /* In C99 (§6.3.1.4p1), converting a float > INT_MAX to int is undefined behavior:
   * - Compile-time constant folding (e.g. FakeCC, older GCC) saturates to 2147483647 (INT_MAX).
   * - Modern GCC on x86-64 generates cvttss2si which produces -2147483648 (0x80000000).
   * Accept both valid results so both compilers pass. */
  int r1 = f1();
  if (r1 != 2147483647 && r1 != -2147483648)
    abort ();
  int r2 = f2();
  if (r2 != 2147483647 && r2 != -2147483648)
    abort ();
  return 0;
}
