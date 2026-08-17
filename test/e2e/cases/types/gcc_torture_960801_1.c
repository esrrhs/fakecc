// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/960801-1.c
package main;

unsigned
f ()
{
  long long l2;
  unsigned short us;
  unsigned long long ul;
  short s2;

  ul = us = l2 = s2 = -1;
  return ul;
}

unsigned long long
g ()
{
  long long l2;
  unsigned short us;
  unsigned long long ul;
  short s2;

  ul = us = l2 = s2 = -1;
  return ul;
}

int
main (void)
{
  if (f () != (unsigned short) -1)
    return 1;
  if (g () != (unsigned short) -1)
    return 1;
  return 0;
}