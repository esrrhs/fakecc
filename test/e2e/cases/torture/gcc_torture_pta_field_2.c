/* pta-field-2.c */

// expect: 0
package main;

struct Foo {
  int *p;
  int *q;
};

static void bar(int **x) {
  struct Foo *f = (struct Foo *)(x - 1);
  *(f->p) = 0;
}

static int foo(void) {
  struct Foo f;
  int i = 1, j = 2;
  f.p = &i;
  f.q = &j;
  bar(&f.q);
  return i;
}

int main(void) {
  if (foo() != 0)
    __builtin_abort();
  return 0;
}
