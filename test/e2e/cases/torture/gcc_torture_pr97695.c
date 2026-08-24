// expect: 0
package main;

int *a, b, **c = &a, d, e;

int f(int g, int h) { return !h || (g && h == 1) ? 0 : g / h; }

static void *i(int g) {
  while (e < 2)
    if (!f(g, 9)) {
      while (b)
        ;
      return 0;
    }
  return 0;
}

void j(void) {
  i(1);
  *c = i(d);
}

int main(void) { j(); return 0; }
