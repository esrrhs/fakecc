/* strncmp-1.c - simplified */

// expect: 0
package main;

int main(void) {
  if (__builtin_strncmp("abc", "abc", 3) != 0)
    __builtin_abort();
  if (__builtin_strncmp("abc", "abd", 3) >= 0)
    __builtin_abort();
  if (__builtin_strncmp("abd", "abc", 3) <= 0)
    __builtin_abort();
  if (__builtin_strncmp("abc", "abc", 0) != 0)
    __builtin_abort();
  if (__builtin_strncmp("abc", "def", 2) >= 0)
    __builtin_abort();
  if (__builtin_strncmp("", "", 1) != 0)
    __builtin_abort();
  return 0;
}
