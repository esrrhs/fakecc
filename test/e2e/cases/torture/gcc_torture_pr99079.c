/* PR tree-optimization/99079 */

// expect: 0
package main;

static unsigned long long foo(int x) {
  unsigned long long s = 1 << x;
  return 4897637220ULL % s;
}

int main(void) {
  if (foo(31) != 4897637220ULL)
    __builtin_abort();
  return 0;
}
