/* pr58570.c - packed bit-field test */

// expect: 0
package main;

struct S {
  int f0:15;
  int f1:29;
};

static int e = 1;
static int i;
static struct S d[6];

int main(void) {
  if (e) {
    d[i].f0 = 1;
    d[i].f1 = 1;
  }
  if (d[0].f1 != 1)
    __builtin_abort();
  return 0;
}
