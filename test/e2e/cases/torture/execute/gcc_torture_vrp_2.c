/* vrp-2.c */

// expect: 0
package main;

static int f(int a) {
  if (a != 2) {
    a = a > 0 ? a : -a;
    if (a == 2)
      return 0;
    return 1;
  }
  return 1;
}

int main(void) {
  if (f(-2))
    __builtin_abort();
  return 0;
}
