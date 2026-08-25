/* PR rtl-optimization/90311 */

// expect: 0
package main;

int a, b;

int main(void) {
  unsigned long long x;
  unsigned int c;
  __builtin_add_overflow((unsigned char)a, b, &c);
  b -= c < (unsigned char)a;
  x = b;
  if (x)
    __builtin_abort();
  return 0;
}
