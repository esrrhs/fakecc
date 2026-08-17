// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010723-1.c
package main;

int
test ()
{
  int biv,giv;
  for (biv = 0, giv = 0; giv != 8; biv++)
      giv = biv*8;
  return giv;
}

int
main(void)
{
  if (test () != 8)
    return 1;
  return 0;
}