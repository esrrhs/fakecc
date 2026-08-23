/* PR rtl-optimization/16536 */

// expect: 0
package main;

typedef struct {
  int i, dummy;
} A;

static inline A foo(const A *p, const A *q) {
  return (A){p->i + q->i};
}

static void bar(A *__restrict__ p) {
  *p = foo(p, p);
  if (p->i != 2)
    __builtin_abort();
}

int main(void) {
  A a = {1};
  bar(&a);
  return 0;
}
