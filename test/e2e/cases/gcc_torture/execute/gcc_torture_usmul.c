/* usmul.c */

// expect: 0
package main;

static int foo(short x, unsigned short y) {
  return x * y;
}

static int bar(unsigned short x, short y) {
  return x * y;
}

int main(void) {
  if (foo(-2, 0xffff) != -131070)
    __builtin_abort();
  if (foo(2, 0xffff) != 131070)
    __builtin_abort();
  if (foo(-32768, 0x8000) != -1073741824)
    __builtin_abort();
  if (foo(32767, 0x8000) != 1073709056)
    __builtin_abort();

  if (bar(0xffff, -2) != -131070)
    __builtin_abort();
  if (bar(0xffff, 2) != 131070)
    __builtin_abort();
  if (bar(0x8000, -32768) != -1073741824)
    __builtin_abort();
  if (bar(0x8000, 32767) != 1073709056)
    __builtin_abort();

  return 0;
}
