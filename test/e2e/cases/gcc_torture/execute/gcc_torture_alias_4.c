// expect: 0
// Mach-O supports only weak aliases; the oracle rejects strong ones.
// skip_oracle: darwin
package main;
// skip_oracle: clang
/* { dg-require-alias "" } */
int a = 1;
extern int b __attribute__ ((alias ("a")));
int c = 1;
extern int d __attribute__ ((alias ("c")));
int
main (int argc)
{
  int *p;
  int *q;
  if (argc)
    p = &a, q = &b;
  else
    p = &c, q = &d;
  *p = 1;
  *q = 2;
  if (*p == 1)
    __builtin_abort ();
  return 0;
}
