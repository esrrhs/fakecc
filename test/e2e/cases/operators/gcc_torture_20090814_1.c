// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20090814-1.c
package main;

int 
bar (int *a)
{
  return *a;
}
int i;
int 
foo (int (*a)[2])
{
  return bar (&(*a)[i]);
}

int a[2];
int main()
{
  a[0] = -1;
  a[1] = 42;
  i = 1;
  if (foo (&a) != 42)
    return 1;
  return 0;
}