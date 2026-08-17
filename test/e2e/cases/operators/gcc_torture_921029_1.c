// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/921029-1.c
package main;

typedef unsigned long long ULL;
ULL back;
ULL hpart, lpart;
ULL
build(long h, long l)
{
  hpart = h;
  hpart <<= 32;
  lpart = l;
  lpart &= 0xFFFFFFFFLL;
  back = hpart | lpart;
  return back;
}

int
main(void)
{
  if (build(0, 1) != 0x0000000000000001LL)
    return 1;
  if (build(0, 0) != 0x0000000000000000LL)
    return 1;
  if (build(0, 0xFFFFFFFF) != 0x00000000FFFFFFFFLL)
    return 1;
  if (build(0, 0xFFFFFFFE) != 0x00000000FFFFFFFELL)
    return 1;
  if (build(1, 1) != 0x0000000100000001LL)
    return 1;
  if (build(1, 0) != 0x0000000100000000LL)
    return 1;
  if (build(1, 0xFFFFFFFF) != 0x00000001FFFFFFFFLL)
    return 1;
  if (build(1, 0xFFFFFFFE) != 0x00000001FFFFFFFELL)
    return 1;
  if (build(0xFFFFFFFF, 1) != 0xFFFFFFFF00000001LL)
    return 1;
  if (build(0xFFFFFFFF, 0) != 0xFFFFFFFF00000000LL)
    return 1;
  if (build(0xFFFFFFFF, 0xFFFFFFFF) != 0xFFFFFFFFFFFFFFFFLL)
    return 1;
  if (build(0xFFFFFFFF, 0xFFFFFFFE) != 0xFFFFFFFFFFFFFFFELL)
    return 1;
  return 0;
}