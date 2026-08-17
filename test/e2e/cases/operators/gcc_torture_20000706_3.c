// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000706-3.c
package main;

int c;

void baz(int *p)
{
  c = *p;
}

int bar(int b)
{
  if (c != 1 || b != 2)
    return 1;

    return 0;}

void foo(int a, int b)
{
  baz(&a);
  bar(b);
}

int main()
{
  foo(1, 2);
  return 0;
}