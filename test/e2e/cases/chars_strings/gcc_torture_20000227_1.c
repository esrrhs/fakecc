// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000227-1.c
package main;

static const unsigned char f[] = "\0\377";
static const unsigned char g[] = "\0\xff";

int main(void)
{
  if (sizeof f != 3 || sizeof g != 3)
    return 1;
  if (f[0] != g[0])
    return 1;
  if (f[1] != g[1])
    return 1;
  if (f[2] != g[2])
    return 1;
  return 0;
}