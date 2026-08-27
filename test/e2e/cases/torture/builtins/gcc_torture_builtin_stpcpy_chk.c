// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/stpcpy-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h stpcpy macros are expanded to
 * __builtin___stpcpy_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-stpcpy is omitted (optimizer-only).
 * test4's struct tag is A4: fakecc rejects two block-scope structs named A.
 * GCC 16 DCE of stores into dead locals needs chk_escape so chk_calls stay
 * live. Original abort / chk_calls checks kept. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern void abort (void);
extern int memcmp (const void *, const void *, size_t);
extern size_t strlen (const char *);
extern char *strcpy (char *, const char *);

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

__attribute__((__noinline__))
char *
stpcpy (char *dst, const char *src)
{
  while (*src != 0)
    *dst++ = *src++;
  *dst = 0;
  return dst;
}

char *
__stpcpy_chk (char *d, const char *s, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (strlen (s) >= size)
    __chk_fail ();
  return stpcpy (d, s);
}

/* GCC 16 may rewrite unused-result stpcpy_chk to strcpy_chk. */
char *
__strcpy_chk (char *d, const char *s, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (strlen (s) >= size)
    __chk_fail ();
  return strcpy (d, s);
}

/* GCC 16+ DCE of stpcpy into dead locals also drops chk_calls.
 * Escape destinations so the original abort checks stay live. */
volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
char *s4;
size_t l1 = 1;
volatile char *vx;

void
__attribute__((noinline))
test1 (void)
{
  int i = 8;

  if (__builtin___stpcpy_chk (p, "abcde", __builtin_object_size (p, 0)) != p + 5 || memcmp (p, "abcde", 6))
    abort ();
  if (__builtin___stpcpy_chk (p + 16, "vwxyz" + 1, __builtin_object_size (p + 16, 0)) != p + 16 + 4 || memcmp (p + 16, "wxyz", 5))
    abort ();
  if (__builtin___stpcpy_chk (p + 1, "", __builtin_object_size (p + 1, 0)) != p + 1 + 0 || memcmp (p, "a\0cde", 6))
    abort ();
  if (__builtin___stpcpy_chk (p + 3, "fghij", __builtin_object_size (p + 3, 0)) != p + 3 + 5 || memcmp (p, "a\0cfghij", 9))
    abort ();

  if (__builtin___stpcpy_chk ((i++, p + 20 + 1), "23", __builtin_object_size ((i++, p + 20 + 1), 0)) != (p + 20 + 1 + 2)
      || i != 9 || memcmp (p + 19, "z\0""23\0", 5))
    abort ();

  if (__builtin___stpcpy_chk (__builtin___stpcpy_chk (p, "ABCD", __builtin_object_size (p, 0)), "EFG", __builtin_object_size (__builtin___stpcpy_chk (p, "ABCD", __builtin_object_size (p, 0)), 0)) != p + 7 || memcmp (p, "ABCDEFG", 8))
    abort();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  if (__builtin___stpcpy_chk (p, "abcde", __builtin_object_size (p, 0)) != p + 5 || memcmp (p, "abcde", 6))
    abort ();

  /* If return value of stpcpy is ignored, it should be optimized into
     strcpy call.  */
  __builtin___stpcpy_chk (p + 1, "abcd", __builtin_object_size (p + 1, 0));
  if (memcmp (p, "aabcd", 6))
    abort ();

  if (chk_calls)
    abort ();

  chk_calls = 0;
  if (__builtin___stpcpy_chk (p, s2, __builtin_object_size (p, 0)) != p + 4 || memcmp (p, "defg\0", 6))
    abort ();
  __builtin___stpcpy_chk (p + 2, s3, __builtin_object_size (p + 2, 0));
  if (memcmp (p, "deFGH", 6))
    abort ();
  if (chk_calls != 2)
    abort ();
}

static union {
  char buf[97];
  long long align_int;
  long double align_fp;
} u1, u2;

void
__attribute__((noinline))
test2 (void)
{
  int off1, off2, len, i;
  char *p, *q, c;

  for (off1 = 0; off1 < 8; off1++)
    for (off2 = 0; off2 < 8; off2++)
      for (len = 1; len < 80; len++)
	{
	  for (i = 0, c = 'A'; i < 97; i++, c++)
	    {
	      u1.buf[i] = 'a';
	      if (c >= 'A' + 31)
		c = 'A';
	      u2.buf[i] = c;
	    }
	  u2.buf[off2 + len] = '\0';

	  p = __builtin___stpcpy_chk (u1.buf + off1, u2.buf + off2, __builtin_object_size (u1.buf + off1, 0));
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

	  if (*q++ != '\0')
	    abort ();
	  for (i = 0; i < 8; i++, q++)
	    if (*q != 'a')
	      abort ();
	}
}

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
  const char *l;
  chk_escape = a.buf1;
  chk_escape = buf3;

  /* The following calls should do runtime checking
     - source length is not known, but destination is.  */
  chk_calls = 0;
  vx = __builtin___stpcpy_chk (a.buf1 + 2, s3 + 3, __builtin_object_size (a.buf1 + 2, 0));
  vx = __builtin___stpcpy_chk (r, s3 + 2, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  vx = __builtin___stpcpy_chk (r, s2 + 2, __builtin_object_size (r, 0));
  vx = __builtin___stpcpy_chk (r + 2, s3 + 3, __builtin_object_size (r + 2, 0));
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
  vx = __builtin___stpcpy_chk (r, s2 + 4, __builtin_object_size (r, 0));
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known source length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  vx = __builtin___stpcpy_chk (a.buf1 + 2, "", __builtin_object_size (a.buf1 + 2, 0));
  vx = __builtin___stpcpy_chk (r, "a", __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  vx = __builtin___stpcpy_chk (r, s1 + 1, __builtin_object_size (r, 0));
  r = buf3;
  l = "abc";
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1], l = "e";
      else if (i == l1)
	r = &a.buf2[7], l = "gh";
      else if (i == l1 + 1)
	r = &buf3[5], l = "jkl";
      else if (i == l1 + 2)
	r = &a.buf1[9], l = "";
    }
  vx = __builtin___stpcpy_chk (r, "", __builtin_object_size (r, 0));
  /* Here, strlen (l) + 1 is known to be at most 4 and
     __builtin_object_size (&buf3[16], 0) is 4, so this doesn't need
     runtime checking.  */
  vx = __builtin___stpcpy_chk (&buf3[16], l, __builtin_object_size (&buf3[16], 0));
  /* Unknown destination and source, no checking.  */
  vx = __builtin___stpcpy_chk (s4, s3, __builtin_object_size (s4, 0));
  __builtin___stpcpy_chk (s4 + 4, s3, __builtin_object_size (s4 + 4, 0));
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
      vx = __builtin___stpcpy_chk (&a.buf2[9], s2 + 3, __builtin_object_size (&a.buf2[9], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      vx = __builtin___stpcpy_chk (&a.buf2[7], s3 + strlen (s3) - 3, __builtin_object_size (&a.buf2[7], 0));
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      vx = __builtin___stpcpy_chk (&buf3[19], "a", __builtin_object_size (&buf3[19], 0));
      abort ();
    }
  chk_fail_allowed = 0;
}

void
main_test (void)
{
  __asm ("" : "=r" (s2) : "0" (s2));
  __asm ("" : "=r" (s3) : "0" (s3));
  __asm ("" : "=r" (l1) : "0" (l1));
  test1 ();
  s4 = p;
  test2 ();
  test3 ();
  test4 ();
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
