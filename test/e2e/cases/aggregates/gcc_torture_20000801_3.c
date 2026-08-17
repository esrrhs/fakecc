// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000801-3.c
package main;

/* Origin: PR c/92 from Simon Marlow <t-simonm@microsoft.com>, adapted
   to a testcase by Joseph Myers <jsm28@cam.ac.uk>.
*/

typedef struct { } empty;

typedef struct {
  int i;
  empty e;
  int i2;
} st;

st s = { .i = 0, .i2 = 1 };

int
main (void)
{
  if (s.i2 == 1)
    return 0;
  else
    return 1;
}