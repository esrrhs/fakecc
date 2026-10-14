// expect: 0
// Mach-O supports only weak aliases; the oracle rejects strong ones.
// skip_oracle: darwin
package main;
// skip_oracle: clang
/* { dg-require-alias "" } */
/* { dg-skip-if "" { powerpc-ibm-aix* } } */
static int a=0;
extern int b __attribute__ ((alias("a")));
__attribute__ ((noinline))
static void inc(void)
{
  b++;
}
int
main()
{
  a=0;
  inc ();
  if (a!=1)
    __builtin_abort ();
  return 0;
}
