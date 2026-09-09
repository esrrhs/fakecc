/* vrp-7.c */

// expect: 0
package main;

struct T {
  int b : 1;
} t;

static void foo(int f) {
  t.b = (f & 0x10) ? 1 : 0;
}

int main(void) {
  foo(0x10);
  if (!t.b)
    __builtin_abort();
  return 0;
}
