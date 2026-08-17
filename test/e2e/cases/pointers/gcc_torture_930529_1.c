// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930529-1.c
package main;

/* { dg-options { "-fwrapv" } } */

int dd (int x, int d) { return x / d; }

int
main ()
{
  int i;
  for (i = -3; i <= 3; i++)
    {
      if (dd (i, 1) != i / 1)
	return 1;
      if (dd (i, 2) != i / 2)
	return 1;
      if (dd (i, 3) != i / 3)
	return 1;
      if (dd (i, 4) != i / 4)
	return 1;
      if (dd (i, 5) != i / 5)
	return 1;
      if (dd (i, 6) != i / 6)
	return 1;
      if (dd (i, 7) != i / 7)
	return 1;
      if (dd (i, 8) != i / 8)
	return 1;
    }
  for (i = ((unsigned) ~0 >> 1) - 3; i <= ((unsigned) ~0 >> 1) + 3; i++)
    {
      if (dd (i, 1) != i / 1)
	return 1;
      if (dd (i, 2) != i / 2)
	return 1;
      if (dd (i, 3) != i / 3)
	return 1;
      if (dd (i, 4) != i / 4)
	return 1;
      if (dd (i, 5) != i / 5)
	return 1;
      if (dd (i, 6) != i / 6)
	return 1;
      if (dd (i, 7) != i / 7)
	return 1;
      if (dd (i, 8) != i / 8)
	return 1;
    }
  return 0;
}