/* pr34982.c - old-style function declaration */

// expect: 0
package main;

static void something(int i) {
  if (i != -1)
    __builtin_abort();
}

int main(void) {
  something(-1);
  return 0;
}
