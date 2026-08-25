/* ssad-run.c */

// expect: 0
package main;

static int foo(signed char *w, int i, signed char *x, int j) {
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

static void bar(signed char *w, signed char *x, int i, int *result) {
  *result = foo(w, 16, x, i);
}

int main(void) {
  signed char m[256];
  signed char n[256];
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

  if (sum != 2368)
    __builtin_abort();

  return 0;
}
