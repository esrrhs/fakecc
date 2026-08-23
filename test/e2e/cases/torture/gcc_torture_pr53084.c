/* PR middle-end/53084 */

// expect: 0
package main;

static void bar(const char *p) {
  if (p[0] != 'o' || p[1] != 'o' || p[2])
    __builtin_abort();
}

int main(void) {
  const char *foo = "foo" + 1;
  bar(foo);
  return 0;
}
