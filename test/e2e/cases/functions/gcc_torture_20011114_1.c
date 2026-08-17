// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20011114-1.c
package main;

char foo(char bar[])
{
  return bar[1];
}
extern char foo(char *);
int main(void)
{
  if (foo("xy") != 'y')
    return 1;
  return 0;
}