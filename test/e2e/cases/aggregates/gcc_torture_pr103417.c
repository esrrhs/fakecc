// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr103417.c
package main;

/* PR tree-optimization/103417 */
/* { dg-require-effective-target int32plus } */

struct { int a : 8; int b : 24; } c = { 0, 1 };

int
main ()
{
  if (c.b && !c.b)
    return 1;
  return 0;
}