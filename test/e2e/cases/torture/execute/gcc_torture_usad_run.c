/* usad-run.c */

// expect: 0
package main;

static int foo(unsigned char *w, int i, unsigned char *x, int j) {
  int tot = 0;
  int a, b;
  for (a = 0; a < 16; a++) {
    for (b = 0; b < 16; b++)
      tot += __builtin_abs(w[b] - x[b]);
    w += i;
    x += j;
  }
  return tot;
}

static void bar(unsigned char *w, unsigned char *x, int i, int *result) {
  *result = foo(w, 16, x, i);
}

int main(void) {
  unsigned char m[256];
  unsigned char n[256];
  int sum, i;

  for (i = 0; i < 256; ++i)
    if (i % 2 == 0) {
      m[i] = (i % 8) * 2 + 1;
      n[i] = -(i % 8);
    } else {
      m[i] = -((i % 8) * 2 + 2);
      n[i] = -((i % 8) >> 1);
    }

  bar(m, n, 16, &sum);

  if (sum != 32384)
    __builtin_abort();

  return 0;
}
