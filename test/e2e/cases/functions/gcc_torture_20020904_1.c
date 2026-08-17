// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020904-1.c
package main;

/* PR c/7102 */

/* Verify that GCC zero-extends integer constants
   in unsigned binary operations. */

typedef unsigned char u8;

u8 fun(u8 y)
{
  u8 x=((u8)255)/y;
  return x;
}

int main(void)
{
  if (fun((u8)2) != 127)
    return 1;
  return 0;
}