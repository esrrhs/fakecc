// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20011128-1.c
package main;

int
main(void)
{
  char blah[33] = "01234567890123456789";
  return 0;
}