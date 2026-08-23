/* PR tree-optimization/59413 */

// expect: 0
package main;

static unsigned int a;
static int b;

int main(void) {
  unsigned int c;
  for (a = 7; a <= 1; a++) {
    char d = a;
    c = d;
    b = a == c;
  }
  if (a != 7)
    __builtin_abort();
  return 0;
}
