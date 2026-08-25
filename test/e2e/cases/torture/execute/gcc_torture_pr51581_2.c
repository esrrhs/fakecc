/* PR tree-optimization/51581-2 - simplified */

// expect: 0
package main;

static int a[64], c[64];
static unsigned int b[64], d[64];

static void f1(void) {
  int i;
  for (i = 0; i < 64; i++)
    c[i] = a[i] % 3;
}

static void f2(void) {
  int i;
  for (i = 0; i < 64; i++)
    d[i] = b[i] % 3;
}

int main(void) {
  int i;
  for (i = 0; i < 64; i++) {
    a[i] = i + 1;
    b[i] = i + 1;
  }
  f1();
  f2();
  for (i = 0; i < 64; i++) {
    if (c[i] != a[i] % 3)
      __builtin_abort();
    if (d[i] != b[i] % 3)
      __builtin_abort();
  }
  return 0;
}
