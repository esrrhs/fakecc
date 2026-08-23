/* PR tree-optimization/93945 - simplified */

// expect: 0
package main;

union U {
  char a[8];
  struct {
    unsigned int b:8;
    unsigned int c:13;
    unsigned int d:11;
  } e;
} u;

static int foo(void) {
  __builtin_memset(&u.a, 0xf4, sizeof(u.a));
  return u.e.c;
}

static int bar(void) {
  return u.e.c;
}

static int baz(void) {
  __builtin_memset(&u.a, 0xf4, sizeof(u.a));
  return u.e.d;
}

static int qux(void) {
  return u.e.d;
}

int main(void) {
  int a = foo();
  int b = bar();
  if (a != b)
    __builtin_abort();
  a = baz();
  b = qux();
  if (a != b)
    __builtin_abort();
  return 0;
}
