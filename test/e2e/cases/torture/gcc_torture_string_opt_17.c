/* string-opt-17.c */

// expect: 0
package main;

static unsigned int test1(char *s, unsigned int i) {
  __builtin_strcpy(s, "foobarbaz" + i++);
  return i;
}

static unsigned int check2(void) {
  static unsigned int r = 5;
  if (r != 5)
    __builtin_abort();
  return ++r;
}

static void test2(char *s) {
  __builtin_strcpy(s, "foobarbaz" + check2());
}

int main(void) {
  char buf[10];
  if (test1(buf, 7) != 8 || __builtin_memcmp(buf, "az", 3))
    __builtin_abort();
  test2(buf);
  if (__builtin_memcmp(buf, "baz", 4))
    __builtin_abort();
  return 0;
}
