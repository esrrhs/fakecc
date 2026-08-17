// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020216-1.c
package main;

/* PR c/3444
   This used to fail because bitwise xor was improperly computed in char type
   and sign extended to int type.  */

signed char c = (signed char) 0xffffffff;

int foo (void)
{
  return (unsigned short) c ^ (signed char) 0x99999999;
}

int main (void)
{
  if ((unsigned char) -1 != 0xff
      || sizeof (short) != 2
      || sizeof (int) != 4)
    return 0;
  if (foo () != (int) 0xffff0066)
    return 1;
  return 0;
}