// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020423-1.c
package main;

/* PR c/5430 */
/* Verify that the multiplicative folding code is not fooled
   by the mix between signed variables and unsigned constants. */

int main (void)
{
  int my_int = 924;
  unsigned int result;

  result = ((my_int*2 + 4) - 8U) / 2;
  if (result != 922U)
    return 1;
         
  result = ((my_int*2 - 4U) + 2) / 2;
  if (result != 923U)
    return 1;

  result = (((my_int + 2) * 2) - 8U - 4) / 2;
  if (result != 920U)
    return 1;
  result = (((my_int + 2) * 2) - (8U + 4)) / 2;
  if (result != 920U)
    return 1;

  result = ((my_int*4 + 2U) - 4U) / 2;
  if (result != 1847U)
    return 1;

  return 0;
}