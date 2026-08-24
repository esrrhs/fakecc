/* struct-aliasing-1.c */

// expect: 0
package main;

struct S { float f; };

static int foo(int *r, struct S *p) {
  int *q = (int *)&p->f;
  int i = *q;
  *r = 0;
  return i + *q;
}

int main(void) {
  int i = 1;
  if (foo(&i, (struct S *)&i) != 1)
    __builtin_abort();
  return 0;
}
