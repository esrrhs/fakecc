/* PR tree-optimization/63641 - simplified */

// expect: 0
package main;

static int foo(unsigned char b) {
  if (0x0 <= b && b <= 0x8)
    return 1;
  if (b == 0x0b)
    return 1;
  if (0x0e <= b && b <= 0x1a)
    return 1;
  if (0x1c <= b && b <= 0x1f)
    return 1;
  return 0;
}

static int bar(unsigned char b) {
  if (0x0 <= b && b <= 0x8)
    return 1;
  if (b == 0x0b)
    return 1;
  if (0x0e <= b && b <= 0x1a)
    return 1;
  if (0x3c <= b && b <= 0x3f)
    return 1;
  return 0;
}

int main(void) {
  int i;
  for (i = 0; i < 256; i++) {
    int expected;
    if (i < 32) {
      static char tab1[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,
                             1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1 };
      expected = tab1[i];
    } else {
      expected = 0;
    }
    if (foo(i) != expected)
      __builtin_abort();
  }
  for (i = 0; i < 256; i++) {
    int expected;
    if (i < 64) {
      static char tab2[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1,
                             1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 };
      expected = tab2[i];
    } else {
      expected = 0;
    }
    if (bar(i) != expected)
      __builtin_abort();
  }
  return 0;
}
