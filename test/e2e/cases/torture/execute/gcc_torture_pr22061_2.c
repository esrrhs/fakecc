// expect: 0
package main;

void exit (int);
int *x;
static void bar (char a[2][(*x)++]) {}
int
main (void)
{
  exit (0);
}
