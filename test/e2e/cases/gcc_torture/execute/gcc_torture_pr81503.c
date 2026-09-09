// expect: 0
package main;

unsigned short a = 41461;
unsigned short b = 3419;
int c = 0;

void foo(void) {
  if (a + b * ~(0 != 5))
    c = -~(b * ~(0 != 5)) + 2147483647;
}

int main(void) {
  foo();
  if (c != 2147476810)
    return -1;
  return 0;
}
