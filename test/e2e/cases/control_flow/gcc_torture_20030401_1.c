// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20030401-1.c
package main;

/* Testcase for PR fortran/9974.  This was a miscompilation of the g77
   front-end caused by the jump bypassing optimizations not handling
   instructions inserted on CFG edges.  */

int bar ()
{
  return 1;
}

int foo (int x)
{
  unsigned char error = 0;

  if (! (error = ((x == 0) || bar ())))
    bar ();
  if (! error)
    return 1;

    return 0;}

int main()
{
  foo (1);
  return 0;
}