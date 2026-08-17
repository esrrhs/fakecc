// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20011109-2.c
package main;

int main(void)
{
  char *c1 = "foo";
  char *c2 = "foo";
  int i;
  for (i = 0; i < 3; i++)
    if (c1[i] != c2[i])
      return 1;
  return 0;
}