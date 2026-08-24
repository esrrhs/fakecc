/* string-opt-18.c */

// expect: 0
package main;

static void test1(void *ptr) {
  char *p = __builtin_memcpy(ptr, ptr, 8);
  if (p != ptr)
    __builtin_abort();
}

static void test3(void *ptr) {
  __builtin_memmove(ptr, ptr, 8);
}

static void test5(void *ptr) {
  if (__builtin_memcmp(ptr, ptr, 8) != 0)
    __builtin_abort();
}

static void test6(const char *ptr) {
  if (__builtin_strcmp(ptr, ptr) != 0)
    __builtin_abort();
}

static void test7(const char *ptr) {
  if (__builtin_strncmp(ptr, ptr, 8) != 0)
    __builtin_abort();
}

int main(void) {
  char buf[10];
  test1(buf);
  test3(buf);
  test5(buf);
  test6(buf);
  test7(buf);
  return 0;
}
