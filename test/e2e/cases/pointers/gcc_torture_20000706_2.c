// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000706-2.c
package main;

struct baz {
  int a, b, c, d, e;
};

int bar(struct baz *x, int f, int g, int h, int i, int j)
{
  if (x->a != 1 || x->b != 2 || x->c != 3 || x->d != 4 || x->e != 5 ||
      f != 6 || g != 7 || h != 8 || i != 9 || j != 10)
    return 1;

    return 0;}

void foo(char *z, struct baz x, char *y)
{
  bar(&x,6,7,8,9,10);
}

int main()
{
  struct baz x;

  x.a = 1;
  x.b = 2;
  x.c = 3;
  x.d = 4;
  x.e = 5;
  foo((char *)0,x,(char *)0);
  return 0;
}