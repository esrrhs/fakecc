// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000706-5.c
package main;

struct baz { int a, b, c; };

struct baz *c;

int bar(int b)
{
  if (c->a != 1 || c->b != 2 || c->c != 3 || b != 4)
    return 1;

    return 0;}

void foo(struct baz a, int b)
{
  c = &a;
  bar(b);
}

int main()
{
  struct baz a;
  a.a = 1;
  a.b = 2;
  a.c = 3;
  foo(a, 4);
  return 0;
}