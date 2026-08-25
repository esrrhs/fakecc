/* vrp-4.c */

// expect: 0
package main;

static void test(int x, int y) {
  int c;

  if (x == 1) __builtin_abort();
  if (y == 1) __builtin_abort();

  c = x / y;

  if (c != 1) __builtin_abort();
}

int main(void) {
  test(2, 2);
  return 0;
}
