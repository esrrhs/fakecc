// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20080408-1.c
package main;

int main ()
{
  short ssi = 126;
  unsigned short usi = 65280;
  int fail = !(ssi < usi);
  if (fail)
    return 1;
  return 0;
}