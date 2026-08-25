/* shiftopt-1.c */

// expect: 0
package main;

static void utest(unsigned int x) {
  if (x >> 0 != x)
    __builtin_abort();
  if (x << 0 != x)
    __builtin_abort();
  if (0 << x != 0)
    __builtin_abort();
  if (0 >> x != 0)
    __builtin_abort();
  if (-1 >> x != -1)
    __builtin_abort();
  if (~0 >> x != ~0)
    __builtin_abort();
}

static void stest(int x) {
  if (x >> 0 != x)
    __builtin_abort();
  if (x << 0 != x)
    __builtin_abort();
  if (0 << x != 0)
    __builtin_abort();
  if (0 >> x != 0)
    __builtin_abort();
}

int main(void) {
  utest(9);
  utest(0);
  stest(9);
  stest(0);
  return 0;
}
