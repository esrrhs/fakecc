// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/stpncpy-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h stpncpy/memset macros are expanded to
 * __builtin___*_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-stpncpy is omitted (optimizer-only).
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
extern void *memset (void *, int, size_t);
extern char *strncpy (char *, const char *, size_t);
extern int memcmp (const void *, const void *, size_t);
extern int strcmp (const char *, const char *);
extern int strncmp (const char *, const char *, size_t);
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
__memset_chk (void *dst, int c, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memset (dst, c, n);
}

__attribute__((__noinline__))
char *
stpncpy (char *dst, const char *src, size_t n)
{
  for (; *src && n; n--)
    *dst++ = *src++;
  {
    char *ret = dst;
    while (n--)
      *dst++ = 0;
    return ret;
  }
}

char *
__stpncpy_chk (char *s1, const char *s2, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return stpncpy (s1, s2, n);
}

/* GCC 16 may rewrite unused-result stpncpy_chk to strncpy_chk. */
char *
__strncpy_chk (char *s1, const char *s2, size_t n, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return strncpy (s1, s2, n);
}

volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
char * volatile s2 = "defg";
char * volatile s3 = "FGH";
char *s4;
volatile size_t l1 = 1;
int i;

void
__attribute__((noinline))
test1 (void)
{
  const char *src = "hello world";
  const char *src2;
  char dst[64], *dst2;

  chk_calls = 0;

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst, src, 4, __builtin_object_size (dst, 0)) != dst+4 || strncmp (dst, src, 4))
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst+16, src, 4, __builtin_object_size (dst+16, 0)) != dst+20 || strncmp (dst+16, src, 4))
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst+32, src+5, 4, __builtin_object_size (dst+32, 0)) != dst+36 || strncmp (dst+32, src+5, 4))
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  dst2 = dst;
  if (__builtin___stpncpy_chk (++dst2, src+5, 4, __builtin_object_size (++dst2, 0)) != dst+5 || strncmp (dst2, src+5, 4)
      || dst2 != dst+1)
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst, src, 0, __builtin_object_size (dst, 0)) != dst || strcmp (dst, ""))
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  dst2 = dst; src2 = src;
  if (__builtin___stpncpy_chk (++dst2, ++src2, 0, __builtin_object_size (++dst2, 0)) != dst+1 || strcmp (dst2, "")
      || dst2 != dst+1 || src2 != src+1)
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  dst2 = dst; src2 = src;
  if (__builtin___stpncpy_chk (++dst2+5, ++src2+5, 0, __builtin_object_size (++dst2+5, 0)) != dst+6 || strcmp (dst2+5, "")
      || dst2 != dst+1 || src2 != src+1)
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst, src, 12, __builtin_object_size (dst, 0)) != dst+11 || strcmp (dst, src))
    abort();

  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst, src, 4, __builtin_object_size (dst, 0)) != dst+4 || strncmp (dst, src, 4))
    abort();

  __builtin___memset_chk (dst, 0, 64, __builtin_object_size (dst, 0));
  if (__builtin___stpncpy_chk (dst, i++ ? "xfoo" + 1 : "bar", 4, __builtin_object_size (dst, 0)) != dst+3
      || strcmp (dst, "bar")
      || i != 1)
    abort ();

  /* If return value of stpncpy is ignored, it should be optimized into
     stpncpy call.  */
  __builtin___stpncpy_chk (dst + 1, src, 4, __builtin_object_size (dst + 1, 0));
  if (strncmp (dst + 1, src, 4))
    abort ();

  if (chk_calls)
    abort ();
}

void
__attribute__((noinline))
test2 (void)
{
  chk_calls = 0;

  /* No runtime checking should be done here, both destination
     and length are unknown.  */
  size_t cpy_length = l1 < 4 ? l1 + 1 : 4;
  if (__builtin___stpncpy_chk (s4, "abcd", l1 + 1, __builtin_object_size (s4, 0)) != s4 + cpy_length || strncmp (s4, "abcd", cpy_length))
    abort ();

  if (chk_calls)
    abort ();
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
  size_t l2;
  chk_escape = a.buf1;
  chk_escape = buf3;

  /* The following calls should do runtime checking
     - source length is not known, but destination is.
     The returned value is checked so that stpncpy calls
     are not rewritten to strncpy calls. */
  chk_calls = 0;
  if (!__builtin___stpncpy_chk (a.buf1 + 2, s3 + 3, l1, __builtin_object_size (a.buf1 + 2, 0)))
    abort();
  if (!__builtin___stpncpy_chk (r, s3 + 2, l1 + 2, __builtin_object_size (r, 0)))
    abort();
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  if (!__builtin___stpncpy_chk (r, s2 + 2, l1 + 2, __builtin_object_size (r, 0)))
    abort();
  if (!__builtin___stpncpy_chk (r + 2, s3 + 3, l1, __builtin_object_size (r + 2, 0)))
    abort();
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
  if (!__builtin___stpncpy_chk (r, s2 + 4, l1, __builtin_object_size (r, 0)))
    abort();
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  if (!__builtin___stpncpy_chk (a.buf1 + 2, "", 3, __builtin_object_size (a.buf1 + 2, 0)))
    abort ();
  if (!__builtin___stpncpy_chk (a.buf1 + 2, "", 0, __builtin_object_size (a.buf1 + 2, 0)))
    abort ();
  if (!__builtin___stpncpy_chk (r, "a", 1, __builtin_object_size (r, 0)))
    abort ();
  if (!__builtin___stpncpy_chk (r, "a", 3, __builtin_object_size (r, 0)))
    abort ();
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  if (!__builtin___stpncpy_chk (r, s1 + 1, 3, __builtin_object_size (r, 0)))
    abort ();
  if (!__builtin___stpncpy_chk (r, s1 + 1, 2, __builtin_object_size (r, 0)))
    abort ();
  r = buf3;
  l = "abc";
  l2 = 4;
  for (i = 0; i < 4; ++i)
    {
      if (i == l1 - 1)
	r = &a.buf1[1], l = "e", l2 = 2;
      else if (i == l1)
	r = &a.buf2[7], l = "gh", l2 = 3;
      else if (i == l1 + 1)
	r = &buf3[5], l = "jkl", l2 = 4;
      else if (i == l1 + 2)
	r = &a.buf1[9], l = "", l2 = 1;
    }
  if (!__builtin___stpncpy_chk (r, "", 1, __builtin_object_size (r, 0)))
    abort ();
  /* Here, strlen (l) + 1 is known to be at most 4 and
     __builtin_object_size (&buf3[16], 0) is 4, so this doesn't need
     runtime checking.  */
  if (!__builtin___stpncpy_chk (&buf3[16], l, l2, __builtin_object_size (&buf3[16], 0)))
    abort ();
  if (!__builtin___stpncpy_chk (&buf3[15], "abc", l2, __builtin_object_size (&buf3[15], 0)))
    abort ();
  if (!__builtin___stpncpy_chk (&buf3[10], "fghij", l2, __builtin_object_size (&buf3[10], 0)))
    abort ();
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
      if (__builtin___stpncpy_chk (&a.buf2[9], s2 + 4, l1 + 1, __builtin_object_size (&a.buf2[9], 0)))
        i++;
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      if (__builtin___stpncpy_chk (&a.buf2[7], s3, l1 + 4, __builtin_object_size (&a.buf2[7], 0)))
        i++;
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      if (__builtin___stpncpy_chk (&buf3[19], "abc", 2, __builtin_object_size (&buf3[19], 0)))
        i++;
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      if (__builtin___stpncpy_chk (&buf3[18], "", 3, __builtin_object_size (&buf3[18], 0)))
        i++;
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
