/* vrp-6.c */

// expect: 0
package main;

static void test01(unsigned int a, unsigned int b) {
  if (a < 5)
    __builtin_abort();
  if (b < 5)
    __builtin_abort();
  if (a - b != 5)
    __builtin_abort();
}

static void test02(unsigned int a, unsigned int b) {
  if (a >= 12)
    if (b > 15)
      if (a - b < 0xffffffffU - 15U)
        __builtin_abort();
}

int main(void) {
  unsigned int x = 0x80000000;
  test01(x + 5, x);
  test02(14, 16);
  return 0;
}
