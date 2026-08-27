// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/strcpy-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h strcpy macros are expanded to
 * __builtin___strcpy_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-strcpy is omitted (optimizer-only).
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
extern void *memcpy (void *, const void *, size_t);
extern char *strcpy (char *, const char *);
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

/* GCC 16 may rewrite strcpy_chk of known strings to memcpy_chk. */
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

volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
char *s4;
size_t l1 = 1;

void
__attribute__((noinline))
test1 (void)
{
  chk_calls = 0;

  if (__builtin___strcpy_chk (p, "abcde", __builtin_object_size (p, 0)) != p || memcmp (p, "abcde", 6))
    abort ();
  if (__builtin___strcpy_chk (p + 16, "vwxyz" + 1, __builtin_object_size (p + 16, 0)) != p + 16 || memcmp (p + 16, "wxyz", 5))
    abort ();
  if (__builtin___strcpy_chk (p + 1, "", __builtin_object_size (p + 1, 0)) != p + 1 || memcmp (p, "a\0cde", 6))
    abort ();
  if (__builtin___strcpy_chk (p + 3, "fghij", __builtin_object_size (p + 3, 0)) != p + 3 || memcmp (p, "a\0cfghij", 9))
    abort ();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  if (__builtin___strcpy_chk (p, "abcde", __builtin_object_size (p, 0)) != p || memcmp (p, "abcde", 6))
    abort ();

  if (chk_calls)
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

	  p = __builtin___strcpy_chk (u1.buf + off1, u2.buf + off2, __builtin_object_size (u1.buf + off1, 0));
	  if (p != u1.buf + off1)
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
  __builtin___strcpy_chk (a.buf1 + 2, s3 + 3, __builtin_object_size (a.buf1 + 2, 0));
  __builtin___strcpy_chk (r, s3 + 2, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___strcpy_chk (r, s2 + 2, __builtin_object_size (r, 0));
  __builtin___strcpy_chk (r + 2, s3 + 3, __builtin_object_size (r + 2, 0));
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
  __builtin___strcpy_chk (r, s2 + 4, __builtin_object_size (r, 0));
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known source length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  __builtin___strcpy_chk (a.buf1 + 2, "", __builtin_object_size (a.buf1 + 2, 0));
  __builtin___strcpy_chk (r, "a", __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___strcpy_chk (r, s1 + 1, __builtin_object_size (r, 0));
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
  __builtin___strcpy_chk (r, "", __builtin_object_size (r, 0));
  /* Here, strlen (l) + 1 is known to be at most 4 and
     __builtin_object_size (&buf3[16], 0) is 4, so this doesn't need
     runtime checking.  */
  __builtin___strcpy_chk (&buf3[16], l, __builtin_object_size (&buf3[16], 0));
  /* Unknown destination and source, no checking.  */
  __builtin___strcpy_chk (s4, s3, __builtin_object_size (s4, 0));
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
      __builtin___strcpy_chk (&a.buf2[9], s2 + 3, __builtin_object_size (&a.buf2[9], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___strcpy_chk (&a.buf2[7], s3 + strlen (s3) - 3, __builtin_object_size (&a.buf2[7], 0));
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___strcpy_chk (&buf3[19], "a", __builtin_object_size (&buf3[19], 0));
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
  test2 ();
  s4 = p;
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
