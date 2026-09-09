/* unroll-1.c */

// expect: 0
package main;

static inline int f(int x) {
  return (x + 1);
}

int main(void) {
  int a = 0;

  while ((f(f(f(f(f(f(f(f(f(f(1))))))))))) + a < 12) {
    a++;
    return 0;
  }
  if (a != 1)
    __builtin_abort();
  return 0;
}
