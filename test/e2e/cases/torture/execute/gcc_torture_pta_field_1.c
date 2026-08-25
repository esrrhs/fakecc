/* pta-field-1.c */

// expect: 0
package main;

struct Foo {
  int *p;
  int *q;
};

static void bar(int **x) {
  struct Foo *f = (struct Foo *)x;
  *(f->q) = 0;
}

static int foo(void) {
  struct Foo f;
  int i = 1, j = 2;
  f.p = &i;
  f.q = &j;
  bar(&f.p);
  return j;
}

int main(void) {
  if (foo() != 0)
    __builtin_abort();
  return 0;
}
