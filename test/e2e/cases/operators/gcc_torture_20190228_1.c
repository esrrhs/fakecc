// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20190228-1.c
package main;

/* PR tree-optimization/89536 */
/* Testcase by Zhendong Su <su@cs.ucdavis.edu> */

int a = 1;

int main (void)
{
  a = ~(a && 1); 
  if (a < -1)
    a = ~a;
  
  if (!a)
    return 1;

  return 0;
}