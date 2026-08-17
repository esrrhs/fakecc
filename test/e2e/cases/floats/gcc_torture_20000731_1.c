// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000731-1.c
package main;

double
foo (void)
{
  return 0.0;
}

void
do_sibcall (void)
{
  (void) foo ();
}

int
main (void)
{
   double x;

   for (x = 0; x < 20; x++)
      do_sibcall ();
   if (!(x >= 10))
      return 1;
   return 0;
}