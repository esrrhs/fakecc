// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/compare-2.c
package main;

/* Copyright (C) 2002 Free Software Foundation.

   Ensure that the composite comparison optimization doesn't misfire
   and attempt to combine a signed comparison with an unsigned one.

   Written by Roger Sayle, 3rd June 2002.  */

int
foo (int x, int y)
{
  /* If miscompiled the following may become "x == y".  */
  return (x<=y) && ((unsigned int)x >= (unsigned int)y);
}

int
main ()
{
  if (! foo (-1,0))
    return 1;
  return 0;
}