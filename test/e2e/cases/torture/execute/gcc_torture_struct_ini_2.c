/* struct-ini-2.c */

// expect: 0
package main;

struct {
  int a:4;
  int b:4;
  int c:4;
} x = {2, 3, 4};

int main(void) {
  if (x.a != 2)
    __builtin_abort();
  if (x.b != 3)
    __builtin_abort();
  if (x.c != 4)
    __builtin_abort();
  return 0;
}
