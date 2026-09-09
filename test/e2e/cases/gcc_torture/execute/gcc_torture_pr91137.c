// expect: 0
package main;

long long a;
unsigned b;
int c[70];
int d[70][70];
int e;

void f(long long *g, int p2) {
  *g = p2;
}

void fn2(void) {
  int j, i, l, k;
  for (j = 0; j < 70; j++) {
    for (i = 0; i < 70; i++) {
      if (b)
        c[i] = 0;
      for (l = 0; l < 70; l++)
        d[i][1] = d[l][i];
    }
    for (k = 0; k < 70; k++)
      e = c[0];
  }
}

int main(void) {
  int j;
  b = 5;
  for (j = 0; j < 70; ++j)
    c[j] = 2075593088;
  fn2();
  f(&a, e);
  if (a)
    __builtin_abort();
  return 0;
}
