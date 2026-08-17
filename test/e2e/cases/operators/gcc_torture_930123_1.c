// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/930123-1.c
package main;

void
f(int *x)
{
  *x = 0;
}

int
main(void)
{
  int s, c, x;
  char a[] = "c";

  f(&s);
  a[c = 0] = s == 0 ? (x=1, 'a') : (x=2, 'b');
  if (a[c] != 'a')
    return 1;
  return 0;
}