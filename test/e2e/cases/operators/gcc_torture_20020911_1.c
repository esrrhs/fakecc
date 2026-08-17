// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020911-1.c
package main;

unsigned short c = 0x8000;
int main()
{
  if ((c-0x8000) < 0 || (c-0x8000) > 0x7fff)
    return 1;
  return 0;
}