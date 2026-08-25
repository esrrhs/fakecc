/* PR target/57568 - simplified */

// expect: 0
package main;

static int a[6][9];
static int b = 1;

int main(void) {
  int *c = &a[3][5];
  if (b && (*c = *c + *c))
    __builtin_abort();
  return 0;
}
