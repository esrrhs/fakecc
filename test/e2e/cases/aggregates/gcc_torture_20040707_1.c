// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040707-1.c
package main;

struct s { char c1, c2; };
void foo (struct s s)
{
  static struct s s1;
  s1 = s;
}
int main ()
{
  static struct s s2;
  foo (s2);
  return 0;
}