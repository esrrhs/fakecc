/* PR tree-optimization/97888 */

// expect: 0
package main;

void foo(int i) {
  if ((i % 7) >= 0) {
    if (i >= 0)
      __builtin_abort();
  }
}

int main(void) {
  foo(-7);
  foo(-21);
  return 0;
}
