/* PR rtl-optimization/97421 */

// expect: 0
package main;

int a, b, d, e;
int *volatile c = &a;

void f(void)
{
  int g;
  for (g = 2; g >= 0; g--) {
    d = 0;
    for (b = 0; b <= 2; b++)
      ;
    e = *c;
  }
}

int main(void)
{
  f();
  if (b != 3)
    __builtin_abort();
  return 0;
}
