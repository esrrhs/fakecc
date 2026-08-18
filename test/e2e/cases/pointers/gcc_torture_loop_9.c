// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/loop-9.c
package main;

/* Source: Neil Booth, from PR # 115.  */
/* { dg-additional-options "-std=gnu17" } */

int false()
{
  return 0;
}

int main (int argc,char *argv[])
{
  int count = 0;

  while (false() || count < -123)
    ++count;

  if (count)
    return 1;

  return 0;
}