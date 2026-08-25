/* struct-ret-2.c */

// expect: 0
package main;

typedef struct {
  unsigned char a __attribute__((packed));
  unsigned short b __attribute__((packed));
} three_byte_t;

static unsigned char f(void) {
  return 0xab;
}

static unsigned short g(void) {
  return 0x1234;
}

int main(void) {
  three_byte_t three_byte;
  three_byte.a = f();
  three_byte.b = g();
  if (three_byte.a != 0xab || three_byte.b != 0x1234)
    __builtin_abort();
  return 0;
}
