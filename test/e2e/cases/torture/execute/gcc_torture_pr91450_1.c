/* PR middle-end/91450 */

// expect: 0
package main;

static unsigned long long foo(int a, int b) {
  unsigned long long r;
  __builtin_mul_overflow(a, b, &r);
  return r;
}

int main(void) {
  if (foo(-4, 2) != -8ULL)
    __builtin_abort();
  if (foo(2, -4) != -8ULL)
    __builtin_abort();
  if (foo(-2, 1) != -2ULL)
    __builtin_abort();
  if (foo(1, -2) != -2ULL)
    __builtin_abort();
  return 0;
}
