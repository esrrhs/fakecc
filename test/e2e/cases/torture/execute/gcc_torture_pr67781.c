// expect: 0
package main;

typedef unsigned uint32_t;
typedef unsigned char uint8_t;

struct
{
  uint32_t a;
  uint8_t b;
} s = { 0x123456, 0x78 };

int pr67781(void)
{
  uint32_t c = (s.a << 8) | s.b;
  return c;
}

int
main (void)
{
  if (pr67781 () != 0x12345678)
    __builtin_abort ();
  return 0;
}
