/* strlen-2.c - simplified */

// expect: 0
package main;

static const char a[2][3] = { "1", "12" };

int main(void) {
  if (__builtin_strlen(a[0]) != 1)
    __builtin_abort();
  if (__builtin_strlen(a[1]) != 2)
    __builtin_abort();
  if (__builtin_strlen(a[0] + 1) != 0)
    __builtin_abort();
  if (__builtin_strlen(a[1] + 1) != 1)
    __builtin_abort();
  return 0;
}
