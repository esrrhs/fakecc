/* PR tree-optimization/94130 - simplified */

// expect: 0
package main;

int main(void) {
  int a[8];
  char *b = __builtin_memset(a, 0, sizeof(a));
  a[0] = 1;
  a[1] = 2;
  a[2] = 3;
  if (b != (char *)a)
    __builtin_abort();
  return 0;
}
