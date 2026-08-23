/* PR target/91635 */

// expect: 0
package main;

unsigned short b, c;
int u, v, w, x;

static int foo(unsigned short c) {
  c <<= __builtin_add_overflow(-c, -1, &b);
  c >>= 1;
  return c;
}

static int bar(unsigned short b) {
  b <<= -14 & 15;
  b = b >> -~1;
  return b;
}

static int baz(unsigned short e) {
  e <<= 1;
  e >>= __builtin_add_overflow(8719476735, u, &v);
  return e;
}

static int qux(unsigned int e) {
  c = ~1;
  c *= e;
  c = c >> (-15 & 5);
  return c + w + x;
}

int main(void) {
  if (foo(0xffff) != 0x7fff)
    __builtin_abort();
  if (bar(5) != 5)
    __builtin_abort();
  if (baz(~0) != 0x7fff)
    __builtin_abort();
  if (qux(2) != 0x7ffe)
    __builtin_abort();
  return 0;
}
