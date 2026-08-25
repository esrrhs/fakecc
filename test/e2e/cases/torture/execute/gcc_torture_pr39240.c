/* PR target/39240 */

// expect: 0
package main;

static int foo1(int x) {
  return x;
}

static unsigned int bar1(int x) {
  return foo1(x + 6);
}

static short int foo2(int x) {
  return x;
}

static unsigned short int bar2(int x) {
  return foo2(x + 6);
}

static signed char foo3(int x) {
  return x;
}

static unsigned char bar3(int x) {
  return foo3(x + 6);
}

static unsigned int foo4(int x) {
  return x;
}

static int bar4(int x) {
  return foo4(x + 6);
}

static unsigned short int foo5(int x) {
  return x;
}

static short int bar5(int x) {
  return foo5(x + 6);
}

static unsigned char foo6(int x) {
  return x;
}

static signed char bar6(int x) {
  return foo6(x + 6);
}

int main(void) {
  if (bar1(-10) != (unsigned int) -4)
    __builtin_abort();
  if (bar2(-10) != (unsigned short int) -4)
    __builtin_abort();
  if (bar3(-10) != (unsigned char) -4)
    __builtin_abort();
  if (bar4(-10) != (int) -4)
    __builtin_abort();
  if (bar5(-10) != (short int) -4)
    __builtin_abort();
  if (bar6(-10) != (signed char) -4)
    __builtin_abort();
  return 0;
}
