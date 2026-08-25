/* PR target/93494 */

// expect: 0
package main;

unsigned short a;

int main(void) {
  unsigned long long y = 0;
  int x = __builtin_add_overflow(y, 0ULL, &a);
  if (x || a)
    __builtin_abort();
  return 0;
}
