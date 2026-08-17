// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000314-2.c
package main;

typedef unsigned long long uint64;
const uint64 bigconst = 1ULL << 34;

int a = 1;

static
uint64 getmask(void)
{
    if (a)
      return bigconst;
    else
      return 0;
}

int
main(void)
{
    uint64 f = getmask();
    if (sizeof (long long) == 8
	&& f != bigconst) return 1;
    return 0;
}