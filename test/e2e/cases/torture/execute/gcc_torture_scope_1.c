/* scope-1.c */

// expect: 0
package main;

int v = 3;

static void f(void) {
  int v = 4;
  {
    extern int v;
    if (v != 3)
      __builtin_abort();
  }
}

int main(void) {
  f();
  return 0;
}
