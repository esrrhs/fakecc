// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040704-1.c
package main;

/* PR 16348: Make sure that condition-first false loops DTRT.  */

int main()
{
  for (; 0 ;)
    {
      return 1;
    label:
      return 0;
    }
  goto label;
}