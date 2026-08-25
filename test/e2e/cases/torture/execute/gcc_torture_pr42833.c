// expect: 0
package main;

typedef signed char int8_t;
typedef unsigned int uint32_t;
typedef int ssize_t;

typedef struct { int8_t v1; int8_t v2; int8_t v3; int8_t v4; } neon_s8;

uint32_t helper_neon_rshl_s8 (uint32_t arg1, uint32_t arg2);

uint32_t
helper_neon_rshl_s8 (uint32_t arg1, uint32_t arg2)
{
  uint32_t res;
  neon_s8 vsrc1;
  neon_s8 vsrc2;
  neon_s8 vdest;
  union { neon_s8 v; uint32_t i; } conv_u;

  conv_u.i = arg1;
  vsrc1 = conv_u.v;
  conv_u.i = arg2;
  vsrc2 = conv_u.v;

  vdest.v1 = vsrc1.v1 << (int8_t)vsrc2.v1;
  vdest.v2 = vsrc1.v2 << (int8_t)vsrc2.v2;
  vdest.v3 = vsrc1.v3 << (int8_t)vsrc2.v3;
  vdest.v4 = vsrc1.v4 << (int8_t)vsrc2.v4;

  conv_u.v = vdest;
  res = conv_u.i;
  return res;
}

extern void abort(void);

int main(void)
{
  uint32_t r = helper_neon_rshl_s8 (0x05050505, 0x01010101);
  if (r != 0x0a0a0a0a)
    abort ();
  return 0;
}
