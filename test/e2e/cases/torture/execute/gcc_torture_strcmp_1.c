/* strcmp-1.c - simplified */

// expect: 0
package main;

void test(const unsigned char *s1, const unsigned char *s2, int expected) {
  int value = __builtin_strcmp((char *)s1, (char *)s2);
  if (expected < 0 && value >= 0)
    __builtin_abort();
  else if (expected == 0 && value != 0)
    __builtin_abort();
  else if (expected > 0 && value <= 0)
    __builtin_abort();
}

int main(void) {
  if (__builtin_strcmp("abc", "abc") != 0)
    __builtin_abort();
  if (__builtin_strcmp("abc", "abd") >= 0)
    __builtin_abort();
  if (__builtin_strcmp("abd", "abc") <= 0)
    __builtin_abort();
  if (__builtin_strcmp("", "") != 0)
    __builtin_abort();
  if (__builtin_strcmp("a", "") <= 0)
    __builtin_abort();
  if (__builtin_strcmp("", "a") >= 0)
    __builtin_abort();
  return 0;
}
