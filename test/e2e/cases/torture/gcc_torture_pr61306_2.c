// expect: 0
package main;

typedef short int16_t;
typedef unsigned uint32_t;

static uint32_t
fake_bswap32 (uint32_t in)
{
  return ((in & 0x000000ffUL) << 24) |
         (((uint32_t)(int16_t)in & 0x00ffff00UL) << 8) |
         ((in & 0x00ff0000UL) >> 8) |
         ((in & 0xff000000UL) >> 24);
}

int main(void)
{
  if (fake_bswap32 (0x81828384) != 0xff838281)
    __builtin_abort ();
  return 0;
}
