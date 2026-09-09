/* strlen-4.c - simplified */

// expect: 0
package main;

static const char a[2][2][5] = { { "1", "12" }, { "123", "1234" } };

int main(void) {
  if (__builtin_strlen(a[0][0]) != 1)
    __builtin_abort();
  if (__builtin_strlen(a[0][1]) != 2)
    __builtin_abort();
  if (__builtin_strlen(a[1][0]) != 3)
    __builtin_abort();
  if (__builtin_strlen(a[1][1]) != 4)
    __builtin_abort();
  return 0;
}
