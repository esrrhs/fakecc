// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001203-1.c
package main;

/* Origin: PR c/410 from Jan Echternach
   <jan.echternach@informatik.uni-rostock.de>,
   adapted to a testcase by Joseph Myers <jsm28@cam.ac.uk>.
*/

static void
foo (void)
{
  struct {
    long a;
    char b[1];
  } x = { 2, { 0 } };
}

int
main (void)
{
  int tmp;
  foo ();
  tmp = 1;
  return 0;
}