// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/950426-2.c
package main;

int
main(void)
{
  long int i = -2147483647L - 1L; /* 0x80000000 */
  char ca = 1;

  if (i >> ca != -1073741824L)
    return 1;

  if (i >> i / -2000000000L != -1073741824L)
    return 1;

  return 0;
}