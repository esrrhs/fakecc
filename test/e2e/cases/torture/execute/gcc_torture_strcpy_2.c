/* strcpy-2.c */

// expect: 0
package main;

static const char a[] = "Hi!THE";

static void f(char *a) {
  __builtin_strcpy(a, "Hi!THE");
}

int main(void) {
  int i;
  char b[7] = {};
  f(b);
  for (i = 0; i < 7; i++) {
    if (a[i] != b[i])
      __builtin_abort();
  }
  return 0;
}
