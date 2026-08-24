/* PR c/58943 - fixed order */

// expect: 0
package main;

static unsigned int x[1] = { 2 };

static unsigned int foo(void) {
  x[0] |= 128;
  return 1;
}

int main(void) {
  unsigned int r = foo();
  x[0] |= r;
  if (x[0] != 131)
    __builtin_abort();
  return 0;
}
