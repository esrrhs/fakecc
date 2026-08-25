/* PR tree-optimization/115154 */

// expect: 0
package main;

struct {
  signed b : 1;
} f;
int i = 55;

static void check(int a) {
  if (!a)
    __builtin_abort();
}

int main(void) {
  int t = i != 5;
  t = t * 5;
  f.b = t;
  check(f.b);
  return 0;
}
