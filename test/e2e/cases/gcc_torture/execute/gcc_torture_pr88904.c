// expect: 0
package main;

/* PR rtl-optimization/88904 */

extern void abort (void);

typedef struct {
  unsigned M1;
  unsigned M2 : 1;
  int : 0;
  unsigned M3 : 1;
} S;

S
foo (void)
{
  S result = {0, 0, 1};
  return result;
}

int
main (void)
{
  S ret = foo ();
  if (ret.M2 != 0)
    abort ();
  if (ret.M3 != 1)
    abort ();
  return 0;
}
