// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/bitfld-signed1-1.c
package main;

/* PR tree-optimization/114666 */
/* We used to miscompile this to be always aborting
   due to the use of the signed 1bit into the COND_EXPR. */

struct {
  signed a : 1;
} b = {-1};
char c;
int main()
{
  if ((b.a ^ 1UL) < 3)
    return 1;
}