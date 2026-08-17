// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991023-1.c
package main;

int blah;

int
foo(void)
{
  int i;

  for (i=0 ; i< 7 ; i++)
    {
      if (i == 7 - 1)
	blah = 0xfcc;
      else
	blah = 0xfee;
    }
  return blah;
}

int
main(void)
{
  if (foo () != 0xfcc)
    return 1;
  return 0;
}