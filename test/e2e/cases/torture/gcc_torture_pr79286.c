/* pr79286.c - simplified */

// expect: 0
package main;

int main(void) {
  int a = 0, c = 0;
  int b;
  int e;
  for (b = 0; b < 4; b++) {
    __builtin_printf("%d\n", b, e);
    while (a && c++)
      e = 0;
  }
  return 0;
}
