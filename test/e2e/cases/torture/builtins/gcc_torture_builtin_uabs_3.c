// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/uabs-3.c (+ lib/main.c).
 * Original abort/link_error checks kept; limits.h macros expanded. */

extern void abort (void);
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
extern void link_error (void);

void
main_test (void)
{
  volatile int i0 = 0, i1 = 1, im1 = -1, imin = -2147483647, imax = 2147483647;
  volatile long l0 = 0L, l1 = 1L, lm1 = -1L, lmin = -9223372036854775807L, lmax = 9223372036854775807L;
  volatile long long ll0 = 0LL, ll1 = 1LL, llm1 = -1LL;
  volatile long long llmin = -0x7fffffffffffffffLL, llmax = 0x7fffffffffffffffLL;
  volatile intmax_t imax0 = 0, imax1 = 1, imaxm1 = -1;
  volatile intmax_t imaxmin = -0x7fffffffffffffffL, imaxmax = 0x7fffffffffffffffL;
  if (__builtin_uabs (i0) != 0)
    abort ();
  if (__builtin_uabs (0) != 0)
    link_error ();
  if (__builtin_uabs (i1) != 1)
    abort ();
  if (__builtin_uabs (1) != 1)
    link_error ();
  if (__builtin_uabs (im1) != 1)
    abort ();
  if (__builtin_uabs (-1) != 1)
    link_error ();
  if (__builtin_uabs (imin) != 2147483647)
    abort ();
  if (__builtin_uabs (imin - 1) != 1U + 2147483647)
    abort ();
  if (__builtin_uabs (-2147483647) != 2147483647)
    link_error ();
  if (__builtin_uabs (-2147483647 - 1) != 1U + 2147483647)
    link_error ();
  if (__builtin_uabs (imax) != 2147483647)
    abort ();
  if (__builtin_uabs (2147483647) != 2147483647)
    link_error ();
  if (__builtin_ulabs (l0) != 0L)
    abort ();
  if (__builtin_ulabs (0L) != 0L)
    link_error ();
  if (__builtin_ulabs (l1) != 1L)
    abort ();
  if (__builtin_ulabs (1L) != 1L)
    link_error ();
  if (__builtin_ulabs (lm1) != 1L)
    abort ();
  if (__builtin_ulabs (-1L) != 1L)
    link_error ();
  if (__builtin_ulabs (lmin) != 9223372036854775807L)
    abort ();
  if (__builtin_ulabs (lmin - 1) != 1UL + 9223372036854775807L)
    abort ();
  if (__builtin_ulabs (-9223372036854775807L) != 9223372036854775807L)
    link_error ();
  if (__builtin_ulabs (-9223372036854775807L - 1) != 1UL + 9223372036854775807L)
    link_error ();
  if (__builtin_ulabs (lmax) != 9223372036854775807L)
    abort ();
  if (__builtin_ulabs (9223372036854775807L) != 9223372036854775807L)
    link_error ();
  if (__builtin_ullabs (ll0) != 0LL)
    abort ();
  if (__builtin_ullabs (0LL) != 0LL)
    link_error ();
  if (__builtin_ullabs (ll1) != 1LL)
    abort ();
  if (__builtin_ullabs (1LL) != 1LL)
    link_error ();
  if (__builtin_ullabs (llm1) != 1LL)
    abort ();
  if (__builtin_ullabs (-1LL) != 1LL)
    link_error ();
  if (__builtin_ullabs (llmin) != 0x7fffffffffffffffLL)
    abort ();
  if (__builtin_ullabs (llmin - 1) != 1ULL + 0x7fffffffffffffffLL)
    abort ();
  if (__builtin_ullabs (-0x7fffffffffffffffLL) != 0x7fffffffffffffffLL)
    link_error ();
  if (__builtin_ullabs (-0x7fffffffffffffffLL - 1) != 1ULL + 0x7fffffffffffffffLL)
    link_error ();
  if (__builtin_ullabs (llmax) != 0x7fffffffffffffffLL)
    abort ();
  if (__builtin_ullabs (0x7fffffffffffffffLL) != 0x7fffffffffffffffLL)
    link_error ();
  if (__builtin_umaxabs (imax0) != 0)
    abort ();
  if (__builtin_umaxabs (0) != 0)
    link_error ();
  if (__builtin_umaxabs (imax1) != 1)
    abort ();
  if (__builtin_umaxabs (1) != 1)
    link_error ();
  if (__builtin_umaxabs (imaxm1) != 1)
    abort ();
  if (__builtin_umaxabs (-1) != 1)
    link_error ();
  if (__builtin_umaxabs (imaxmin) != 0x7fffffffffffffffL)
    abort ();
  if (__builtin_umaxabs (imaxmin - 1) != (uintmax_t) 1 + 0x7fffffffffffffffL)
    abort ();
  if (__builtin_umaxabs (-0x7fffffffffffffffL) != 0x7fffffffffffffffL)
    link_error ();
  if (__builtin_umaxabs (-0x7fffffffffffffffL - 1) != (uintmax_t) 1 + 0x7fffffffffffffffL)
    link_error ();
  if (__builtin_umaxabs (imaxmax) != 0x7fffffffffffffffL)
    abort ();
  if (__builtin_umaxabs (0x7fffffffffffffffL) != 0x7fffffffffffffffL)
    link_error ();
}

int main (void)
{
  main_test ();
  return 0;
}

void link_error (void)
{
  abort ();
}
