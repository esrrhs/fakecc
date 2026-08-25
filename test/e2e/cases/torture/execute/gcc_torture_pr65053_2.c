/* PR tree-optimization/65053-2 - simplified */

// expect: 0
package main;

static int i;

int main(void) {
  unsigned int n = 0;
  unsigned int u = 32;
  if (n >= 32)
    __builtin_abort();
  if (n != 0)
    u = n + 32;

  while (u != 32) {
    u = 32;
    i = 1;
  }

  if (i)
    __builtin_abort();
  return 0;
}
