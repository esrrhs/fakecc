// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20051104-1.c
package main;

/* PR rtl-optimization/23567 */

struct
{
  int len;
  char *name;
} s;

int
main (void)
{
  s.len = 0;
  s.name = "";
  if (s.name [s.len] != 0)
    s.name [s.len] = 0;
  return 0;
}