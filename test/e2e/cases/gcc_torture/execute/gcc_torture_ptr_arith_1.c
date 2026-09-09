/* ptr-arith-1.c */

// expect: 0
package main;

static char *f(char *s, unsigned int i) {
  return &s[i + 3 - 1];
}

int main(void) {
  char *str = "abcdefghijkl";
  char *x2 = f(str, 12);
  if (str + 14 != x2)
    __builtin_abort();
  return 0;
}
