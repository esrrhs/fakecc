// expect: 0
package main;

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);

extern void abort (void);
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
__attribute__ ((__noinline__))
int
abs (int x)
{
  do { } while (0);
  return x < 0 ? -x : x;
}
__attribute__ ((__noinline__))
long
labs (long x)
{
  do { } while (0);
  return x < 0 ? -x : x;
}
__attribute__ ((__noinline__))
long long
llabs (long long x)
{
  do { } while (0);
  return x < 0 ? -x : x;
}
__attribute__ ((__noinline__))
intmax_t
imaxabs (intmax_t x)
{
  do { } while (0);
  return x < 0 ? -x : x;
}
__attribute__ ((__noinline__))
unsigned int
uabs (int x)
{
  do { } while (0);
  return x < 0 ? -(unsigned int) x : x;
}
__attribute__ ((__noinline__))
unsigned long
ulabs (long x)
{
  do { } while (0);
  return x < 0 ? -(unsigned long) x : x;
}
__attribute__ ((__noinline__))
unsigned long long
ullabs (long long x)
{
  do { } while (0);
  return x < 0 ? -(unsigned long long) x : x;
}
__attribute__ ((__noinline__))
uintmax_t
umaxabs (intmax_t x)
{
  do { } while (0);
  return x < 0 ? -(uintmax_t) x : x;
}
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
extern unsigned int uabs (int);
extern unsigned long ulabs (long);
extern unsigned long long ullabs (long long);
extern uintmax_t umaxabs (intmax_t);
extern void abort (void);
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
  if (uabs (i0) != 0)
    abort ();
  if (uabs (0) != 0)
    link_error ();
  if (uabs (i1) != 1)
    abort ();
  if (uabs (1) != 1)
    link_error ();
  if (uabs (im1) != 1)
    abort ();
  if (uabs (-1) != 1)
    link_error ();
  if (uabs (imin) != 2147483647)
    abort ();
  if (uabs (imin - 1) != 1U + 2147483647)
    abort ();
  if (uabs (-2147483647) != 2147483647)
    link_error ();
  if (uabs (-2147483647 - 1) != 1U + 2147483647)
    link_error ();
  if (uabs (imax) != 2147483647)
    abort ();
  if (uabs (2147483647) != 2147483647)
    link_error ();
  if (ulabs (l0) != 0L)
    abort ();
  if (ulabs (0L) != 0L)
    link_error ();
  if (ulabs (l1) != 1L)
    abort ();
  if (ulabs (1L) != 1L)
    link_error ();
  if (ulabs (lm1) != 1L)
    abort ();
  if (ulabs (-1L) != 1L)
    link_error ();
  if (ulabs (lmin) != 9223372036854775807L)
    abort ();
  if (ulabs (lmin - 1) != 1UL + 9223372036854775807L)
    abort ();
  if (ulabs (-9223372036854775807L) != 9223372036854775807L)
    link_error ();
  if (ulabs (-9223372036854775807L - 1) != 1UL + 9223372036854775807L)
    link_error ();
  if (ulabs (lmax) != 9223372036854775807L)
    abort ();
  if (ulabs (9223372036854775807L) != 9223372036854775807L)
    link_error ();
  if (ullabs (ll0) != 0LL)
    abort ();
  if (ullabs (0LL) != 0LL)
    link_error ();
  if (ullabs (ll1) != 1LL)
    abort ();
  if (ullabs (1LL) != 1LL)
    link_error ();
  if (ullabs (llm1) != 1LL)
    abort ();
  if (ullabs (-1LL) != 1LL)
    link_error ();
  if (ullabs (llmin) != 0x7fffffffffffffffLL)
    abort ();
  if (ullabs (llmin - 1) != 1ULL + 0x7fffffffffffffffLL)
    abort ();
  if (ullabs (-0x7fffffffffffffffLL) != 0x7fffffffffffffffLL)
    link_error ();
  if (ullabs (-0x7fffffffffffffffLL - 1) != 1ULL + 0x7fffffffffffffffLL)
    link_error ();
  if (ullabs (llmax) != 0x7fffffffffffffffLL)
    abort ();
  if (ullabs (0x7fffffffffffffffLL) != 0x7fffffffffffffffLL)
    link_error ();
  if (umaxabs (imax0) != 0)
    abort ();
  if (umaxabs (0) != 0)
    link_error ();
  if (umaxabs (imax1) != 1)
    abort ();
  if (umaxabs (1) != 1)
    link_error ();
  if (umaxabs (imaxm1) != 1)
    abort ();
  if (umaxabs (-1) != 1)
    link_error ();
  if (umaxabs (imaxmin) != 0x7fffffffffffffffL)
    abort ();
  if (umaxabs (imaxmin - 1) != (uintmax_t) 1 + 0x7fffffffffffffffL)
    abort ();
  if (umaxabs (-0x7fffffffffffffffL) != 0x7fffffffffffffffL)
    link_error ();
  if (umaxabs (-0x7fffffffffffffffL - 1) != (uintmax_t) 1 + 0x7fffffffffffffffL)
    link_error ();
  if (umaxabs (imaxmax) != 0x7fffffffffffffffL)
    abort ();
  if (umaxabs (0x7fffffffffffffffL) != 0x7fffffffffffffffL)
    link_error ();
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}

void link_error (void)
{
  abort ();
}
