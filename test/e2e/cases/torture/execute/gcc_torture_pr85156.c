/* PR tree-optimization/85156 */

// expect: 0
package main;

int x, y;

static int foo(int z) {
  if (__builtin_expect(x ? y != 0 : 0, z++))
    return 7;
  return z;
}

int main(void) {
  x = 1;
  if (foo(10) != 11)
    __builtin_abort();
  return 0;
}
