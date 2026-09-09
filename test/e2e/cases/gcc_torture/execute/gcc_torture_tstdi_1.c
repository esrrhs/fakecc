/* tstdi-1.c */

// expect: 0
package main;

static int feq(long long int x) {
  if (x == 0)
    return 13;
  else
    return 140;
}

static int fne(long long int x) {
  if (x != 0)
    return 13;
  else
    return 140;
}

static int flt(long long int x) {
  if (x < 0)
    return 13;
  else
    return 140;
}

static int fge(long long int x) {
  if (x >= 0)
    return 13;
  else
    return 140;
}

static int fgt(long long int x) {
  if (x > 0)
    return 13;
  else
    return 140;
}

static int fle(long long int x) {
  if (x <= 0)
    return 13;
  else
    return 140;
}

int main(void) {
  if (feq(0LL) != 13)
    __builtin_abort();
  if (feq(-1LL) != 140)
    __builtin_abort();
  if (feq(0x8000000000000000LL) != 140)
    __builtin_abort();
  if (feq(0x8000000000000001LL) != 140)
    __builtin_abort();
  if (feq(1LL) != 140)
    __builtin_abort();
  if (feq(0x7fffffffffffffffLL) != 140)
    __builtin_abort();

  if (fne(0LL) != 140)
    __builtin_abort();
  if (fne(-1LL) != 13)
    __builtin_abort();
  if (fne(0x8000000000000000LL) != 13)
    __builtin_abort();
  if (fne(0x8000000000000001LL) != 13)
    __builtin_abort();
  if (fne(1LL) != 13)
    __builtin_abort();
  if (fne(0x7fffffffffffffffLL) != 13)
    __builtin_abort();

  if (flt(0LL) != 140)
    __builtin_abort();
  if (flt(-1LL) != 13)
    __builtin_abort();
  if (flt(0x8000000000000000LL) != 13)
    __builtin_abort();
  if (flt(0x8000000000000001LL) != 13)
    __builtin_abort();
  if (flt(1LL) != 140)
    __builtin_abort();
  if (flt(0x7fffffffffffffffLL) != 140)
    __builtin_abort();

  if (fge(0LL) != 13)
    __builtin_abort();
  if (fge(-1LL) != 140)
    __builtin_abort();
  if (fge(0x8000000000000000LL) != 140)
    __builtin_abort();
  if (fge(0x8000000000000001LL) != 140)
    __builtin_abort();
  if (fge(1LL) != 13)
    __builtin_abort();
  if (fge(0x7fffffffffffffffLL) != 13)
    __builtin_abort();

  if (fgt(0LL) != 140)
    __builtin_abort();
  if (fgt(-1LL) != 140)
    __builtin_abort();
  if (fgt(0x8000000000000000LL) != 140)
    __builtin_abort();
  if (fgt(0x8000000000000001LL) != 140)
    __builtin_abort();
  if (fgt(1LL) != 13)
    __builtin_abort();
  if (fgt(0x7fffffffffffffffLL) != 13)
    __builtin_abort();

  if (fle(0LL) != 13)
    __builtin_abort();
  if (fle(-1LL) != 13)
    __builtin_abort();
  if (fle(0x8000000000000000LL) != 13)
    __builtin_abort();
  if (fle(0x8000000000000001LL) != 13)
    __builtin_abort();
  if (fle(1LL) != 140)
    __builtin_abort();
  if (fle(0x7fffffffffffffffLL) != 140)
    __builtin_abort();

  return 0;
}
