/* strlen-3.c - simplified */

// expect: 0
package main;

static const char a[2][3][9] = {
  { "1", "1\0002" },
  { "12\0003", "123\0004" }
};

int main(void) {
  if (__builtin_strlen(a[0][0]) != 1)
    __builtin_abort();
  if (__builtin_strlen(a[0][1]) != 1)
    __builtin_abort();
  if (__builtin_strlen(a[1][0]) != 2)
    __builtin_abort();
  if (__builtin_strlen(a[1][1]) != 3)
    __builtin_abort();
  return 0;
}
