/* PR tree-optimization/65053-1 - simplified */

// expect: 0
package main;

static int i;

static unsigned int foo(void) {
  return 0;
}

int main(void) {
  unsigned int u = -1;
  if (u == -1) {
    unsigned int n = foo();
    if (n > 0)
      u = n - 1;
  }

  while (u != -1) {
    u = -1;
    i = 1;
  }

  if (i)
    __builtin_abort();
  return 0;
}
