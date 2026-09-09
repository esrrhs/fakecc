// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/mempcpy-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h mempcpy/memcpy/memset macros are expanded to
 * __builtin___*_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-mempcpy is omitted (optimizer-only).
 * test4's struct tag is A4: fakecc rejects two block-scope structs named A.
 * GCC 16 DCE of copies into dead locals needs chk_escape so chk_calls stay
 * live. GCC 16 may rewrite unused-result mempcpy_chk into __memcpy_chk, so
 * that runtime is included too. Original abort / chk_calls checks kept. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern void abort (void);
extern void *memcpy (void *, const void *, size_t);
extern void *memset (void *, int, size_t);
extern int memcmp (const void *, const void *, size_t);
extern size_t strlen (const char *);

void *chk_fail_buf[256] __attribute__((aligned (16)));
volatile int chk_fail_allowed, chk_calls;
volatile int memcpy_disallowed, mempcpy_disallowed, memmove_disallowed;
volatile int memset_disallowed, strcpy_disallowed, stpcpy_disallowed;
volatile int strncpy_disallowed, stpncpy_disallowed, strcat_disallowed;
volatile int strncat_disallowed, sprintf_disallowed, vsprintf_disallowed;
volatile int snprintf_disallowed, vsnprintf_disallowed;

void __attribute__((noreturn))
__chk_fail (void)
{
  if (chk_fail_allowed)
    __builtin_longjmp (chk_fail_buf, 1);
  abort ();
}

void *
mempcpy (void *dst, const void *src, size_t n)
{
  memcpy (dst, src, n);
  return (char *) dst + n;
}

void *
__mempcpy_chk (void *dst, const void *src, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return mempcpy (dst, src, n);
}

void *
__memcpy_chk (void *dst, const void *src, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memcpy (dst, src, n);
}

void *
__memset_chk (void *dst, int c, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memset (dst, c, n);
}

/* GCC 16+ DCE's copies into dead locals, which also drops __*_chk /
 * chk_calls.  Escape destinations so the original abort checks stay live. */
volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
volatile char *s2 = "defg"; /* prevent constant propagation to happen when whole program assumptions are made.  */
volatile char *s3 = "FGH"; /* prevent constant propagation to happen when whole program assumptions are made.  */
volatile size_t l1 = 1; /* prevent constant propagation to happen when whole program assumptions are made.  */

void
__attribute__((noinline))
test1 (void)
{
  int i;

  mempcpy_disallowed = 1;

  /* All the mempcpy calls in this routine except last have fixed length, so
     object size checking should be done at compile time if optimizing.  */
  chk_calls = 0;

  if (__builtin___mempcpy_chk (p, "ABCDE", 6, __builtin_object_size (p, 0)) != p + 6 || memcmp (p, "ABCDE", 6))
    abort ();
  if (__builtin___mempcpy_chk (p + 16, "VWX" + 1, 2, __builtin_object_size (p + 16, 0)) != p + 16 + 2
      || memcmp (p + 16, "WX\0\0", 5))
    abort ();
  if (__builtin___mempcpy_chk (p + 1, "", 1, __builtin_object_size (p + 1, 0)) != p + 1 + 1 || memcmp (p, "A\0CDE", 6))
    abort ();
  if (__builtin___mempcpy_chk (p + 3, "FGHI", 4, __builtin_object_size (p + 3, 0)) != p + 3 + 4 || memcmp (p, "A\0CFGHI", 8))
    abort ();

  i = 8;
  __builtin___memcpy_chk (p + 20, "qrstu", 6, __builtin_object_size (p + 20, 0));
  __builtin___memcpy_chk (p + 25, "QRSTU", 6, __builtin_object_size (p + 25, 0));
  if (__builtin___mempcpy_chk (p + 25 + 1, s1, 3, __builtin_object_size (p + 25 + 1, 0)) != (p + 25 + 1 + 3)
      || memcmp (p + 25, "Q123U", 6))
    abort ();

  if (__builtin___mempcpy_chk (__builtin___mempcpy_chk (p, "abcdEFG", 4, __builtin_object_size (p, 0)), "efg", 4, __builtin_object_size (__builtin___mempcpy_chk (p, "abcdEFG", 4, __builtin_object_size (p, 0)), 0)) != p + 8
      || memcmp (p, "abcdefg", 8))
    abort();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  if (__builtin___mempcpy_chk (p, "ABCDE", 6, __builtin_object_size (p, 0)) != p + 6 || memcmp (p, "ABCDE", 6))
    abort ();

  /* If the result of mempcpy is ignored, gcc should use memcpy.
     This should be optimized always, so disallow mempcpy calls.  */
  mempcpy_disallowed = 1;
  __builtin___mempcpy_chk (p + 5, s3, 1, __builtin_object_size (p + 5, 0));
  if (memcmp (p, "ABCDEFg", 8))
    abort ();

  if (chk_calls)
    abort ();
  chk_calls = 0;

  __builtin___mempcpy_chk (p + 6, s1 + 1, l1, __builtin_object_size (p + 6, 0));
  if (memcmp (p, "ABCDEF2", 8))
    abort ();

  /* The above mempcpy copies into an object with known size, but
     unknown length and with result ignored, so it should be a
     __memcpy_chk call.  */
  if (chk_calls != 1)
    abort ();

  mempcpy_disallowed = 0;
}

long buf1[64];
char *buf2 = (char *) (buf1 + 32);
long buf5[20];
char buf7[20];

void
__attribute__((noinline))
test2_sub (long *buf3, char *buf4, char *buf6, int n)
{
  int i = 0;

  /* All the mempcpy/__builtin_mempcpy/__builtin___mempcpy_chk
     calls in this routine are either fixed length, or have
     side-effects in __builtin_object_size arguments, or
     dst doesn't point into a known object.  */
  chk_calls = 0;

  /* These should probably be handled by store_by_pieces on most arches.  */
  if (__builtin___mempcpy_chk (buf1, "ABCDEFGHI", 9, __builtin_object_size (buf1, 0)) != (char *) buf1 + 9
      || memcmp (buf1, "ABCDEFGHI\0", 11))
    abort ();

  if (__builtin___mempcpy_chk (buf1, "abcdefghijklmnopq", 17, __builtin_object_size (buf1, 0)) != (char *) buf1 + 17
      || memcmp (buf1, "abcdefghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf3, "ABCDEF", 6, __builtin_object_size (buf3, 0)) != (char *) buf1 + 6
      || memcmp (buf1, "ABCDEFghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf3, "a", 1, __builtin_object_size (buf3, 0)) != (char *) buf1 + 1
      || memcmp (buf1, "aBCDEFghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk ((char *) buf3 + 2, "bcd" + ++i, 2, __builtin_object_size ((char *) buf3 + 2, 0)) != (char *) buf1 + 4
      || memcmp (buf1, "aBcdEFghijklmnopq\0", 19)
      || i != 1)
    abort ();

  /* These should probably be handled by move_by_pieces on most arches.  */
  if (__builtin___mempcpy_chk ((char *) buf3 + 4, buf5, 6, __builtin_object_size ((char *) buf3 + 4, 0)) != (char *) buf1 + 10
      || memcmp (buf1, "aBcdRSTUVWklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk ((char *) buf1 + ++i + 8, (char *) buf5 + 1, 1, __builtin_object_size ((char *) buf1 + ++i + 8, 0))
      != (char *) buf1 + 11
      || memcmp (buf1, "aBcdRSTUVWSlmnopq\0", 19)
      || i != 2)
    abort ();

  if (__builtin___mempcpy_chk ((char *) buf3 + 14, buf6, 2, __builtin_object_size ((char *) buf3 + 14, 0)) != (char *) buf1 + 16
      || memcmp (buf1, "aBcdRSTUVWSlmnrsq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf3, buf5, 8, __builtin_object_size (buf3, 0)) != (char *) buf1 + 8
      || memcmp (buf1, "RSTUVWXYVWSlmnrsq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf3, buf5, 17, __builtin_object_size (buf3, 0)) != (char *) buf1 + 17
      || memcmp (buf1, "RSTUVWXYZ01234567\0", 19))
    abort ();

  __builtin___memcpy_chk (buf3, "aBcdEFghijklmnopq\0", 19, __builtin_object_size (buf3, 0));

  /* These should be handled either by movmemendM or mempcpy
     call.  */

  /* buf3 points to an unknown object, so __mempcpy_chk should not be done.  */
  if (__builtin___mempcpy_chk ((char *) buf3 + 4, buf5, n + 6, __builtin_object_size ((char *) buf3 + 4, 0)) != (char *) buf1 + 10
      || memcmp (buf1, "aBcdRSTUVWklmnopq\0", 19))
    abort ();

  /* This call has side-effects in dst, therefore no checking.  */
  if (__builtin___mempcpy_chk ((char *) buf1 + ++i + 8, (char *) buf5 + 1, n + 1, __builtin_object_size ((char *) buf1 + ++i + 8, 0))
      != (char *) buf1 + 12
      || memcmp (buf1, "aBcdRSTUVWkSmnopq\0", 19)
      || i != 3)
    abort ();

  if (__builtin___mempcpy_chk ((char *) buf3 + 14, buf6, n + 2, __builtin_object_size ((char *) buf3 + 14, 0)) != (char *) buf1 + 16
      || memcmp (buf1, "aBcdRSTUVWkSmnrsq\0", 19))
    abort ();

  i = 1;

  /* These might be handled by store_by_pieces.  */
  if (__builtin___mempcpy_chk (buf2, "ABCDEFGHI", 9, __builtin_object_size (buf2, 0)) != buf2 + 9
      || memcmp (buf2, "ABCDEFGHI\0", 11))
    abort ();

  if (__builtin___mempcpy_chk (buf2, "abcdefghijklmnopq", 17, __builtin_object_size (buf2, 0)) != buf2 + 17
      || memcmp (buf2, "abcdefghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf4, "ABCDEF", 6, __builtin_object_size (buf4, 0)) != buf2 + 6
      || memcmp (buf2, "ABCDEFghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf4, "a", 1, __builtin_object_size (buf4, 0)) != buf2 + 1
      || memcmp (buf2, "aBCDEFghijklmnopq\0", 19))
    abort ();

  if (__builtin___mempcpy_chk (buf4 + 2, "bcd" + i++, 2, __builtin_object_size (buf4 + 2, 0)) != buf2 + 4
      || memcmp (buf2, "aBcdEFghijklmnopq\0", 19)
      || i != 2)
    abort ();

  /* These might be handled by move_by_pieces.  */
  if (__builtin___mempcpy_chk (buf4 + 4, buf7, 6, __builtin_object_size (buf4 + 4, 0)) != buf2 + 10
      || memcmp (buf2, "aBcdRSTUVWklmnopq\0", 19))
    abort ();

  /* Side effect.  */
  if (__builtin___mempcpy_chk (buf2 + i++ + 8, buf7 + 1, 1, __builtin_object_size (buf2 + i++ + 8, 0))
      != buf2 + 11
      || memcmp (buf2, "aBcdRSTUVWSlmnopq\0", 19)
      || i != 3)
    abort ();

  if (__builtin___mempcpy_chk (buf4 + 14, buf6, 2, __builtin_object_size (buf4 + 14, 0)) != buf2 + 16
      || memcmp (buf2, "aBcdRSTUVWSlmnrsq\0", 19))
    abort ();

  __builtin___memcpy_chk (buf4, "aBcdEFghijklmnopq\0", 19, __builtin_object_size (buf4, 0));

  /* These should be handled either by movmemendM or mempcpy
     call.  */
  if (__builtin___mempcpy_chk (buf4 + 4, buf7, n + 6, __builtin_object_size (buf4 + 4, 0)) != buf2 + 10
      || memcmp (buf2, "aBcdRSTUVWklmnopq\0", 19))
    abort ();

  /* Side effect.  */
  if (__builtin___mempcpy_chk (buf2 + i++ + 8, buf7 + 1, n + 1, __builtin_object_size (buf2 + i++ + 8, 0))
      != buf2 + 12
      || memcmp (buf2, "aBcdRSTUVWkSmnopq\0", 19)
      || i != 4)
    abort ();

  if (__builtin___mempcpy_chk (buf4 + 14, buf6, n + 2, __builtin_object_size (buf4 + 14, 0)) != buf2 + 16
      || memcmp (buf2, "aBcdRSTUVWkSmnrsq\0", 19))
    abort ();

  if (chk_calls)
    abort ();
}

void
__attribute__((noinline))
test2 (void)
{
  long *x;
  char *y;
  int z;
  __builtin___memcpy_chk (buf5, "RSTUVWXYZ0123456789", 20, __builtin_object_size (buf5, 0));
  __builtin___memcpy_chk (buf7, "RSTUVWXYZ0123456789", 20, __builtin_object_size (buf7, 0));
 __asm ("" : "=r" (x) : "0" (buf1));
 __asm ("" : "=r" (y) : "0" (buf2));
 __asm ("" : "=r" (z) : "0" (0));
  test2_sub (x, y, "rstuvwxyz", z);
}

volatile void *vx;

/* Test whether compile time checking is done where it should
   and so is runtime object size checking.  */
void
__attribute__((noinline))
test3 (void)
{
  struct A { char buf1[10]; char buf2[10]; } a;
  char *r = l1 == 1 ? &a.buf1[5] : &a.buf2[4];
  char buf3[20];
  int i;
  size_t l;
  chk_escape = a.buf1;
  chk_escape = buf3;

  /* The following calls should do runtime checking
     - length is not known, but destination is.  */
  chk_calls = 0;
  vx = __builtin___mempcpy_chk (a.buf1 + 2, s3, l1, __builtin_object_size (a.buf1 + 2, 0));
  vx = __builtin___mempcpy_chk (r, s3, l1 + 1, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  vx = __builtin___mempcpy_chk (r, s2, l1 + 2, __builtin_object_size (r, 0));
  vx = __builtin___mempcpy_chk (r + 2, s3, l1, __builtin_object_size (r + 2, 0));
  r = buf3;
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1];
      else if (i == l1)
	r = &a.buf2[7];
      else if (i == l1 + 1)
	r = &buf3[5];
      else if (i == l1 + 2)
	r = &a.buf1[9];
    }
  vx = __builtin___mempcpy_chk (r, s2, l1, __builtin_object_size (r, 0));
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  vx = __builtin___mempcpy_chk (a.buf1 + 2, s3, 1, __builtin_object_size (a.buf1 + 2, 0));
  vx = __builtin___mempcpy_chk (r, s3, 2, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  vx = __builtin___mempcpy_chk (r, s2, 3, __builtin_object_size (r, 0));
  r = buf3;
  l = 4;
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1], l = 2;
      else if (i == l1)
	r = &a.buf2[7], l = 3;
      else if (i == l1 + 1)
	r = &buf3[5], l = 4;
      else if (i == l1 + 2)
	r = &a.buf1[9], l = 1;
    }
  vx = __builtin___mempcpy_chk (r, s2, 1, __builtin_object_size (r, 0));
  /* Here, l is known to be at most 4 and __builtin_object_size (&buf3[16], 0)
     is 4, so this doesn't need runtime checking.  */
  vx = __builtin___mempcpy_chk (&buf3[16], s2, l, __builtin_object_size (&buf3[16], 0));
  if (chk_calls)
    abort ();
  chk_calls = 0;
}

/* Test whether runtime and/or compile time checking catches
   buffer overflows.  */
void
__attribute__((noinline))
test4 (void)
{
  struct A4 { char buf1[10]; char buf2[10]; } a;
  char buf3[20];
  chk_escape = a.buf1;
  chk_escape = buf3;

  chk_fail_allowed = 1;
  /* Runtime checks.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      vx = __builtin___mempcpy_chk (&a.buf2[9], s2, l1 + 1, __builtin_object_size (&a.buf2[9], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      vx = __builtin___mempcpy_chk (&a.buf2[7], s3, strlen (s3) + 1, __builtin_object_size (&a.buf2[7], 0));
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      vx = __builtin___mempcpy_chk (&buf3[19], "ab", 2, __builtin_object_size (&buf3[19], 0));
      abort ();
    }
  chk_fail_allowed = 0;
}

static union {
  char buf[96];
  long long align_int;
  long double align_fp;
} u1, u2;

void
__attribute__((noinline))
test5 (void)
{
  int off1, off2, len, i;
  char *p, *q, c;

  for (off1 = 0; off1 < 8; off1++)
    for (off2 = 0; off2 < 8; off2++)
      for (len = 1; len < 80; len++)
	{
	  for (i = 0, c = 'A'; i < 96; i++, c++)
	    {
	      u1.buf[i] = 'a';
	      if (c >= 'A' + 31)
		c = 'A';
	      u2.buf[i] = c;
	    }

	  p = __builtin___mempcpy_chk (u1.buf + off1, u2.buf + off2, len, __builtin_object_size (u1.buf + off1, 0));
	  if (p != u1.buf + off1 + len)
	    abort ();

	  q = u1.buf;
	  for (i = 0; i < off1; i++, q++)
	    if (*q != 'a')
	      abort ();

	  for (i = 0, c = 'A' + off2; i < len; i++, q++, c++)
	    {
	      if (c >= 'A' + 31)
		c = 'A';
	      if (*q != c)
		abort ();
	    }

	  for (i = 0; i < 8; i++, q++)
	    if (*q != 'a')
	      abort ();
	}
}

char srcb[80] __attribute__ ((aligned));
char dstb[80] __attribute__ ((aligned));

void
__attribute__((noinline))
check (char *test, char *match, int n)
{
  if (memcmp (test, match, n))
    abort ();
}

void
__attribute__((noinline))
test6 (void)
{
  int i;

  chk_calls = 0;

  for (i = 0; i < sizeof (srcb); ++i)
      srcb[i] = 'a' + i % 26;

  { __builtin___memset_chk (dstb, 0, 0, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 0, __builtin_object_size (dstb, 0)); check (dstb, srcb, 0); }
  { __builtin___memset_chk (dstb, 0, 1, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 1, __builtin_object_size (dstb, 0)); check (dstb, srcb, 1); }
  { __builtin___memset_chk (dstb, 0, 2, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 2, __builtin_object_size (dstb, 0)); check (dstb, srcb, 2); }
  { __builtin___memset_chk (dstb, 0, 3, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 3, __builtin_object_size (dstb, 0)); check (dstb, srcb, 3); }
  { __builtin___memset_chk (dstb, 0, 4, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 4, __builtin_object_size (dstb, 0)); check (dstb, srcb, 4); }
  { __builtin___memset_chk (dstb, 0, 5, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 5, __builtin_object_size (dstb, 0)); check (dstb, srcb, 5); }
  { __builtin___memset_chk (dstb, 0, 6, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 6, __builtin_object_size (dstb, 0)); check (dstb, srcb, 6); }
  { __builtin___memset_chk (dstb, 0, 7, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 7, __builtin_object_size (dstb, 0)); check (dstb, srcb, 7); }
  { __builtin___memset_chk (dstb, 0, 8, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 8, __builtin_object_size (dstb, 0)); check (dstb, srcb, 8); }
  { __builtin___memset_chk (dstb, 0, 9, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 9, __builtin_object_size (dstb, 0)); check (dstb, srcb, 9); }
  { __builtin___memset_chk (dstb, 0, 10, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 10, __builtin_object_size (dstb, 0)); check (dstb, srcb, 10); }
  { __builtin___memset_chk (dstb, 0, 11, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 11, __builtin_object_size (dstb, 0)); check (dstb, srcb, 11); }
  { __builtin___memset_chk (dstb, 0, 12, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 12, __builtin_object_size (dstb, 0)); check (dstb, srcb, 12); }
  { __builtin___memset_chk (dstb, 0, 13, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 13, __builtin_object_size (dstb, 0)); check (dstb, srcb, 13); }
  { __builtin___memset_chk (dstb, 0, 14, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 14, __builtin_object_size (dstb, 0)); check (dstb, srcb, 14); }
  { __builtin___memset_chk (dstb, 0, 15, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 15, __builtin_object_size (dstb, 0)); check (dstb, srcb, 15); }
  { __builtin___memset_chk (dstb, 0, 16, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 16, __builtin_object_size (dstb, 0)); check (dstb, srcb, 16); }
  { __builtin___memset_chk (dstb, 0, 17, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 17, __builtin_object_size (dstb, 0)); check (dstb, srcb, 17); }
  { __builtin___memset_chk (dstb, 0, 18, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 18, __builtin_object_size (dstb, 0)); check (dstb, srcb, 18); }
  { __builtin___memset_chk (dstb, 0, 19, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 19, __builtin_object_size (dstb, 0)); check (dstb, srcb, 19); }
  { __builtin___memset_chk (dstb, 0, 20, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 20, __builtin_object_size (dstb, 0)); check (dstb, srcb, 20); }
  { __builtin___memset_chk (dstb, 0, 21, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 21, __builtin_object_size (dstb, 0)); check (dstb, srcb, 21); }
  { __builtin___memset_chk (dstb, 0, 22, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 22, __builtin_object_size (dstb, 0)); check (dstb, srcb, 22); }
  { __builtin___memset_chk (dstb, 0, 23, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 23, __builtin_object_size (dstb, 0)); check (dstb, srcb, 23); }
  { __builtin___memset_chk (dstb, 0, 24, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 24, __builtin_object_size (dstb, 0)); check (dstb, srcb, 24); }
  { __builtin___memset_chk (dstb, 0, 25, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 25, __builtin_object_size (dstb, 0)); check (dstb, srcb, 25); }
  { __builtin___memset_chk (dstb, 0, 26, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 26, __builtin_object_size (dstb, 0)); check (dstb, srcb, 26); }
  { __builtin___memset_chk (dstb, 0, 27, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 27, __builtin_object_size (dstb, 0)); check (dstb, srcb, 27); }
  { __builtin___memset_chk (dstb, 0, 28, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 28, __builtin_object_size (dstb, 0)); check (dstb, srcb, 28); }
  { __builtin___memset_chk (dstb, 0, 29, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 29, __builtin_object_size (dstb, 0)); check (dstb, srcb, 29); }
  { __builtin___memset_chk (dstb, 0, 30, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 30, __builtin_object_size (dstb, 0)); check (dstb, srcb, 30); }
  { __builtin___memset_chk (dstb, 0, 31, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 31, __builtin_object_size (dstb, 0)); check (dstb, srcb, 31); }
  { __builtin___memset_chk (dstb, 0, 32, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 32, __builtin_object_size (dstb, 0)); check (dstb, srcb, 32); }
  { __builtin___memset_chk (dstb, 0, 33, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 33, __builtin_object_size (dstb, 0)); check (dstb, srcb, 33); }
  { __builtin___memset_chk (dstb, 0, 34, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 34, __builtin_object_size (dstb, 0)); check (dstb, srcb, 34); }
  { __builtin___memset_chk (dstb, 0, 35, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 35, __builtin_object_size (dstb, 0)); check (dstb, srcb, 35); }
  { __builtin___memset_chk (dstb, 0, 36, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 36, __builtin_object_size (dstb, 0)); check (dstb, srcb, 36); }
  { __builtin___memset_chk (dstb, 0, 37, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 37, __builtin_object_size (dstb, 0)); check (dstb, srcb, 37); }
  { __builtin___memset_chk (dstb, 0, 38, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 38, __builtin_object_size (dstb, 0)); check (dstb, srcb, 38); }
  { __builtin___memset_chk (dstb, 0, 39, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 39, __builtin_object_size (dstb, 0)); check (dstb, srcb, 39); }
  { __builtin___memset_chk (dstb, 0, 40, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 40, __builtin_object_size (dstb, 0)); check (dstb, srcb, 40); }
  { __builtin___memset_chk (dstb, 0, 41, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 41, __builtin_object_size (dstb, 0)); check (dstb, srcb, 41); }
  { __builtin___memset_chk (dstb, 0, 42, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 42, __builtin_object_size (dstb, 0)); check (dstb, srcb, 42); }
  { __builtin___memset_chk (dstb, 0, 43, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 43, __builtin_object_size (dstb, 0)); check (dstb, srcb, 43); }
  { __builtin___memset_chk (dstb, 0, 44, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 44, __builtin_object_size (dstb, 0)); check (dstb, srcb, 44); }
  { __builtin___memset_chk (dstb, 0, 45, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 45, __builtin_object_size (dstb, 0)); check (dstb, srcb, 45); }
  { __builtin___memset_chk (dstb, 0, 46, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 46, __builtin_object_size (dstb, 0)); check (dstb, srcb, 46); }
  { __builtin___memset_chk (dstb, 0, 47, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 47, __builtin_object_size (dstb, 0)); check (dstb, srcb, 47); }
  { __builtin___memset_chk (dstb, 0, 48, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 48, __builtin_object_size (dstb, 0)); check (dstb, srcb, 48); }
  { __builtin___memset_chk (dstb, 0, 49, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 49, __builtin_object_size (dstb, 0)); check (dstb, srcb, 49); }
  { __builtin___memset_chk (dstb, 0, 50, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 50, __builtin_object_size (dstb, 0)); check (dstb, srcb, 50); }
  { __builtin___memset_chk (dstb, 0, 51, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 51, __builtin_object_size (dstb, 0)); check (dstb, srcb, 51); }
  { __builtin___memset_chk (dstb, 0, 52, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 52, __builtin_object_size (dstb, 0)); check (dstb, srcb, 52); }
  { __builtin___memset_chk (dstb, 0, 53, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 53, __builtin_object_size (dstb, 0)); check (dstb, srcb, 53); }
  { __builtin___memset_chk (dstb, 0, 54, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 54, __builtin_object_size (dstb, 0)); check (dstb, srcb, 54); }
  { __builtin___memset_chk (dstb, 0, 55, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 55, __builtin_object_size (dstb, 0)); check (dstb, srcb, 55); }
  { __builtin___memset_chk (dstb, 0, 56, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 56, __builtin_object_size (dstb, 0)); check (dstb, srcb, 56); }
  { __builtin___memset_chk (dstb, 0, 57, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 57, __builtin_object_size (dstb, 0)); check (dstb, srcb, 57); }
  { __builtin___memset_chk (dstb, 0, 58, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 58, __builtin_object_size (dstb, 0)); check (dstb, srcb, 58); }
  { __builtin___memset_chk (dstb, 0, 59, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 59, __builtin_object_size (dstb, 0)); check (dstb, srcb, 59); }
  { __builtin___memset_chk (dstb, 0, 60, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 60, __builtin_object_size (dstb, 0)); check (dstb, srcb, 60); }
  { __builtin___memset_chk (dstb, 0, 61, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 61, __builtin_object_size (dstb, 0)); check (dstb, srcb, 61); }
  { __builtin___memset_chk (dstb, 0, 62, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 62, __builtin_object_size (dstb, 0)); check (dstb, srcb, 62); }
  { __builtin___memset_chk (dstb, 0, 63, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 63, __builtin_object_size (dstb, 0)); check (dstb, srcb, 63); }
  { __builtin___memset_chk (dstb, 0, 64, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 64, __builtin_object_size (dstb, 0)); check (dstb, srcb, 64); }
  { __builtin___memset_chk (dstb, 0, 65, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 65, __builtin_object_size (dstb, 0)); check (dstb, srcb, 65); }
  { __builtin___memset_chk (dstb, 0, 66, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 66, __builtin_object_size (dstb, 0)); check (dstb, srcb, 66); }
  { __builtin___memset_chk (dstb, 0, 67, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 67, __builtin_object_size (dstb, 0)); check (dstb, srcb, 67); }
  { __builtin___memset_chk (dstb, 0, 68, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 68, __builtin_object_size (dstb, 0)); check (dstb, srcb, 68); }
  { __builtin___memset_chk (dstb, 0, 69, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 69, __builtin_object_size (dstb, 0)); check (dstb, srcb, 69); }
  { __builtin___memset_chk (dstb, 0, 70, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 70, __builtin_object_size (dstb, 0)); check (dstb, srcb, 70); }
  { __builtin___memset_chk (dstb, 0, 71, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 71, __builtin_object_size (dstb, 0)); check (dstb, srcb, 71); }
  { __builtin___memset_chk (dstb, 0, 72, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 72, __builtin_object_size (dstb, 0)); check (dstb, srcb, 72); }
  { __builtin___memset_chk (dstb, 0, 73, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 73, __builtin_object_size (dstb, 0)); check (dstb, srcb, 73); }
  { __builtin___memset_chk (dstb, 0, 74, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 74, __builtin_object_size (dstb, 0)); check (dstb, srcb, 74); }
  { __builtin___memset_chk (dstb, 0, 75, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 75, __builtin_object_size (dstb, 0)); check (dstb, srcb, 75); }
  { __builtin___memset_chk (dstb, 0, 76, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 76, __builtin_object_size (dstb, 0)); check (dstb, srcb, 76); }
  { __builtin___memset_chk (dstb, 0, 77, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 77, __builtin_object_size (dstb, 0)); check (dstb, srcb, 77); }
  { __builtin___memset_chk (dstb, 0, 78, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 78, __builtin_object_size (dstb, 0)); check (dstb, srcb, 78); }
  { __builtin___memset_chk (dstb, 0, 79, __builtin_object_size (dstb, 0)); vx = __builtin___mempcpy_chk (dstb, srcb, 79, __builtin_object_size (dstb, 0)); check (dstb, srcb, 79); }

  /* All mempcpy calls in this routine have constant arguments.  */
  if (chk_calls)
    abort ();
}

void
main_test (void)
{
  __asm ("" : "=r" (l1) : "0" (l1));
  test1 ();
  test2 ();
  test3 ();
  test4 ();
  test5 ();
  test6 ();
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
