/* PR tree-optimization/88693 */

// expect: 0
package main;

static void foo(char *p) {
  if (__builtin_strlen(p) != 9)
    __builtin_abort();
}

static void quux(char *p) {
  int i;
  for (i = 0; i < 100; i++)
    if (p[i] != 'x')
      __builtin_abort();
}

static void qux(void) {
  char b[100];
  __builtin_memset(b, 'x', sizeof(b));
  quux(b);
}

static void bar(void) {
  static unsigned char u[9] = "abcdefghi";
  char b[100];
  __builtin_memcpy(b, u, sizeof(u));
  b[sizeof(u)] = 0;
  foo(b);
}

static void baz(void) {
  static unsigned char u[] = {'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r'};
  char b[100];
  __builtin_memcpy(b, u, sizeof(u));
  b[sizeof(u)] = 0;
  foo(b);
}

int main(void) {
  qux();
  bar();
  baz();
  return 0;
}
