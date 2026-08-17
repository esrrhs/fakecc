// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr101335.c
package main;

unsigned a = 0xFFFFFFFF;
int b;
int main()
{
  int c = ~a;
  unsigned d = c - 10;
  if (d > c)
    c = 20;
  b = -(c | 0);
  if (b > -8)
    return 1;
  return 0;
}