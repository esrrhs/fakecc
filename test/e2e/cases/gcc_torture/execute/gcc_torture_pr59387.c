/* PR tree-optimization/59387 */

// expect: 0
package main;

static int a, *d, **e = &d, f;
static char c;
static struct S { int f1; } b;

int main(void) {
  for (a = -19; a; a++) {
    for (b.f1 = 0; b.f1 < 24; b.f1++)
      c--;
    *e = &f;
    if (!d)
      return 0;
  }
  return 0;
}
