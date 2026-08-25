/* pr59747.c - simplified */

// expect: 0
package main;

static int a[6], c = 1, d;
static short e;

static int fn1(int p) {
  return a[p];
}

int main(void) {
  a[0] = 1;
  if (c)
    e--;
  d = e;
  long long f = e;
  if (fn1((f >> 56) & 1) != 0)
    __builtin_abort();
  return 0;
}
