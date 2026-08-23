/* string-opt-5.c */

// expect: 0
package main;

static char *bar = "hi world";

int main(void) {
  const char *const foo = "hello world";

  if (__builtin_strlen(bar) != 8)
    __builtin_abort();
  if (__builtin_strcmp(foo, bar) >= 0)
    __builtin_abort();
  if (__builtin_strchr(bar, 'o') != bar + 4)
    __builtin_abort();
  if (__builtin_strchr(bar, '\0') != bar + 8)
    __builtin_abort();
  if (__builtin_strrchr(bar, 'x'))
    __builtin_abort();
  if (__builtin_strrchr(bar, 'o') != bar + 4)
    __builtin_abort();
  return 0;
}
