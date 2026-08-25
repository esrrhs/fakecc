/* widechar-1.c */

// expect: 0
package main;

static wchar_t x[2] = { 196, 0 };
static wchar_t y = 196;

int main(void) {
  if (sizeof(x) / sizeof(wchar_t) != 2)
    __builtin_abort();
  if (x[0] != y || x[1] != 0)
    __builtin_abort();
  if (y != x[0])
    __builtin_abort();
  return 0;
}
