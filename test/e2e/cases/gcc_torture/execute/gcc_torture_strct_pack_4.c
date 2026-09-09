/* strct-pack-4.c */

// expect: 0
package main;

typedef struct {
  unsigned char a __attribute__((packed));
  unsigned short b __attribute__((packed));
} three_char_t;

static unsigned char my_set_a(void) {
  return 0xab;
}

static unsigned short my_set_b(void) {
  return 0x1234;
}

int main(void) {
  three_char_t three_char;
  three_char.a = my_set_a();
  three_char.b = my_set_b();
  if (three_char.a != 0xab || three_char.b != 0x1234)
    __builtin_abort();
  return 0;
}
