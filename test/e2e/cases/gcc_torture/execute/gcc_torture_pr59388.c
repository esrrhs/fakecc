/* PR tree-optimization/59388 */

// expect: 0
package main;

static int a;
static struct S { unsigned int f:1; } b;

int main(void) {
  a = (0 < b.f) | b.f;
  return a;
}
