/* PR tree-optimization/46309 - simplified */

// expect: 0
package main;

static unsigned int *q;

static void bar(unsigned int *p) {
  if (*p != 2 && *p != 3) {
    if (!(*q & 263)) {
      if (*p != 1)
        __builtin_abort();
    }
  }
}

int main(void) {
  unsigned int x, y;
  x = 2;
  bar(&x);
  x = 3;
  bar(&x);
  y = 1;
  x = 0;
  q = &y;
  bar(&x);
  y = 0;
  x = 1;
  bar(&x);
  return 0;
}
