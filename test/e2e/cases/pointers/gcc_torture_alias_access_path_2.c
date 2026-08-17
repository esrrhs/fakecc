// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/alias-access-path-2.c
package main;

int
main (int argc, char **argv)
{
  int c;
  unsigned char out[][1] = { {71}, {71}, {71} };

  for (int i = 0; i < 3; i++)
    if (!out[i][0])
      return 1;
  return 0;
}