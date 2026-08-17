// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020805-1.c
package main;

/* This testcase was miscompiled on IA-32, because fold-const
   assumed associate_trees is always done on PLUS_EXPR.  */

int check (unsigned int m)
{
  if (m != (unsigned int) -1)
    return 1;

    return 0;}

unsigned int n = 1;

int main (void)
{
  unsigned int m;
  m = (1 | (2 - n)) | (-n);
  check (m);
  return 0;
}