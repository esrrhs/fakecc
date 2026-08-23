/* PR target/71554 */

// expect: 0
package main;

static int v;

static void bar(void) {
  v++;
}

static void foo(unsigned int x) {
  signed int y = ((-2147483647 - 1) / 2);
  signed int r;
  if (__builtin_mul_overflow(x, y, &r))
    bar();
}

int main(void) {
  foo(2);
  if (v)
    __builtin_abort();
  return 0;
}
