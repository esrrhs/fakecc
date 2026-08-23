/* PR target/60454 */

// expect: 0
package main;

typedef unsigned uint32_t;

static uint32_t
fake_swap32 (uint32_t in)
{
  return ((in & 0x000000ffUL) << 24) |
         ((in & 0x0000ff00UL) <<  8) |
         ((in & 0x000000ffUL) <<  8) |
         ((in & 0x0000ff00UL)      ) |
         ((in & 0xff000000UL) >> 24);
}

int main(void)
{
  if (fake_swap32 (0x12345678UL) != 0x78567E12UL)
    __builtin_abort ();
  return 0;
}
