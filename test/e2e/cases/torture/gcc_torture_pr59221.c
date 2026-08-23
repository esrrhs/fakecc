/* pr59221.c */

// expect: 0
package main;

static int a = 1, b, d;
static short e;

int main(void) {
  for (; b; b++)
    ;
  short f = a;
  int g = 15;
  e = f ? f : 1 << g;
  int h = e;
  d = h == 83647 ? 0 : h;
  if (d != 1)
    __builtin_abort();
  return 0;
}
