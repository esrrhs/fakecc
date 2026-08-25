/* vrp-5.c */

// expect: 0
package main;

static void test(unsigned int a, unsigned int b) {
  if (a < 5)
    __builtin_abort();
  if (b < 5)
    __builtin_abort();
  if (a + b != 0U)
    __builtin_abort();
}

int main(void) {
  unsigned int x = 0x80000000;
  test(x, x);
  return 0;
}
