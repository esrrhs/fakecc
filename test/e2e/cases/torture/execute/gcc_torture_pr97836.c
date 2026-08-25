// expect: 0
package main;

int a;

int b(int c) { return 0; }

static int *d(int *e) {
  if (a) {
    a = a && b(*e);
  }
  return e;
}

int main(void) {
  int f;
  if (d(&f) != &f)
    __builtin_abort();
  return 0;
}
