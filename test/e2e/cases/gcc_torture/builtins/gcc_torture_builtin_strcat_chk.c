// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/strcat-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h strcat/strcpy/memset macros are expanded to
 * __builtin___*_chk(..., __builtin_object_size(dst, 0)). RESET_DST_WITH is
 * expanded. GCC lib inside_main/__OPTIMIZE__ abort-on-strcat is omitted
 * (optimizer-only). test3's struct tag is A3: fakecc rejects two block-scope
 * structs named A. GCC 16 DCE of stores into dead locals needs chk_escape so
 * chk_calls stay live. Original abort / chk_calls checks kept. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern void abort (void);
extern void *memset (void *, int, size_t);
extern char *strcpy (char *, const char *);
extern int memcmp (const void *, const void *, size_t);
extern int strcmp (const char *, const char *);
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

__attribute__((__noinline__))
char *
strcat (char *dst, const char *src)
{
  char *p = dst;
  while (*p)
    p++;
  while ((*p++ = *src++))
    ;
  return dst;
}

char *
__strcat_chk (char *d, const char *s, size_t size)
{
  if (size == (size_t) -1)
    abort ();
  ++chk_calls;
  if (strlen (d) + strlen (s) >= size)
    __chk_fail ();
  return strcat (d, s);
}

volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
char *s4;
size_t l1 = 1;
char *s5;

static void
reset_dst_with (char *dst, const char *filler)
{
  __builtin___memset_chk (dst, 'X', 64, __builtin_object_size (dst, 0));
  __builtin___strcpy_chk (dst, filler, __builtin_object_size (dst, 0));
}

void
__attribute__((noinline))
test1 (void)
{
  const char *x1 = "hello world";
  const char *x2 = "";
  char dst[64], *d2;

  chk_calls = 0;
  /* Following strcat calls should be optimized out at compile time.  */
  reset_dst_with (dst, x1);
  if (__builtin___strcat_chk (dst, "", __builtin_object_size (dst, 0)) != dst || strcmp (dst, x1))
    abort ();
  reset_dst_with (dst, x1);
  if (__builtin___strcat_chk (dst, x2, __builtin_object_size (dst, 0)) != dst || strcmp (dst, x1))
    abort ();
  reset_dst_with (dst, x1); d2 = dst;
  if (__builtin___strcat_chk (++d2, x2, __builtin_object_size (++d2, 0)) != dst+1 || d2 != dst+1 || strcmp (dst, x1))
    abort ();
  reset_dst_with (dst, x1); d2 = dst;
  if (__builtin___strcat_chk (++d2+5, x2, __builtin_object_size (++d2+5, 0)) != dst+6 || d2 != dst+1 || strcmp (dst, x1))
    abort ();
  reset_dst_with (dst, x1); d2 = dst;
  if (__builtin___strcat_chk (++d2+5, x1+11, __builtin_object_size (++d2+5, 0)) != dst+6 || d2 != dst+1 || strcmp (dst, x1))
    abort ();
  if (chk_calls)
    abort ();

  reset_dst_with (dst, x1);
  if (__builtin___strcat_chk (dst, " 1111", __builtin_object_size (dst, 0)) != dst
      || memcmp (dst, "hello world 1111\0XXX", 20))
    abort ();

  reset_dst_with (dst, x1);
  if (__builtin___strcat_chk (dst+5, " 2222", __builtin_object_size (dst+5, 0)) != dst+5
      || memcmp (dst, "hello world 2222\0XXX", 20))
    abort ();

  reset_dst_with (dst, x1); d2 = dst;
  if (__builtin___strcat_chk (++d2+5, " 3333", __builtin_object_size (++d2+5, 0)) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world 3333\0XXX", 20))
    abort ();

  reset_dst_with (dst, x1);
  __builtin___strcat_chk (
    __builtin___strcat_chk (
      __builtin___strcat_chk (
        __builtin___strcat_chk (
          __builtin___strcat_chk (
            __builtin___strcat_chk (dst, ": this ", __builtin_object_size (dst, 0)),
            "", __builtin_object_size (dst, 0)),
          "is ", __builtin_object_size (dst, 0)),
        "a ", __builtin_object_size (dst, 0)),
      "test", __builtin_object_size (dst, 0)),
    ".", __builtin_object_size (dst, 0));
  if (memcmp (dst, "hello world: this is a test.\0X", 30))
    abort ();

  chk_calls = 0;
  /* Test at least one instance of the __builtin_ style.  We do this
     to ensure that it works and that the prototype is correct.  */
  reset_dst_with (dst, x1);
  if (__builtin___strcat_chk (dst, "", __builtin_object_size (dst, 0)) != dst || strcmp (dst, x1))
    abort ();
  if (chk_calls)
    abort ();
}

/* Test whether compile time checking is done where it should
   and so is runtime object size checking.  */
void
__attribute__((noinline))
test2 (void)
{
  struct A { char buf1[10]; char buf2[10]; } a;
  char *r = l1 == 1 ? &a.buf1[5] : &a.buf2[4];
  char buf3[20];
  int i;
  chk_escape = a.buf1;
  chk_escape = buf3;

  /* The following calls should do runtime checking
     - source length is not known, but destination is.  */
  __builtin___memset_chk (&a, '\0', 20, __builtin_object_size (&a, 0));
  s5 = (char *) &a;
  __asm __volatile ("" : : "r" (s5) : "memory");
  chk_calls = 0;
  __builtin___strcat_chk (a.buf1 + 2, s3 + 3, __builtin_object_size (a.buf1 + 2, 0));
  __builtin___strcat_chk (r, s3 + 2, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___memset_chk (r, '\0', 3, __builtin_object_size (r, 0));
  __asm __volatile ("" : : "r" (r) : "memory");
  __builtin___strcat_chk (r, s2 + 2, __builtin_object_size (r, 0));
  __builtin___strcat_chk (r + 2, s3 + 3, __builtin_object_size (r + 2, 0));
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
  __builtin___strcat_chk (r, s2 + 4, __builtin_object_size (r, 0));
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known source length,
     but we don't know the length of dest string, so runtime checking
     is needed too.  */
  __builtin___memset_chk (&a, '\0', 20, __builtin_object_size (&a, 0));
  chk_calls = 0;
  s5 = (char *) &a;
  __asm __volatile ("" : : "r" (s5) : "memory");
  __builtin___strcat_chk (a.buf1 + 2, "a", __builtin_object_size (a.buf1 + 2, 0));
  __builtin___strcat_chk (r, "", __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___memset_chk (r, '\0', 3, __builtin_object_size (r, 0));
  __asm __volatile ("" : : "r" (r) : "memory");
  __builtin___strcat_chk (r, s1 + 1, __builtin_object_size (r, 0));
  if (chk_calls != 2)
    abort ();
  chk_calls = 0;
  /* Unknown destination and source, no checking.  */
  __builtin___strcat_chk (s4, s3, __builtin_object_size (s4, 0));
  if (chk_calls)
    abort ();
  chk_calls = 0;
}

/* Test whether runtime and/or compile time checking catches
   buffer overflows.  */
void
__attribute__((noinline))
test3 (void)
{
  struct A3 { char buf1[10]; char buf2[10]; } a;
  char buf3[20];
  chk_escape = a.buf1;
  chk_escape = buf3;

  __builtin___memset_chk (&a, '\0', 20, __builtin_object_size (&a, 0));
  __builtin___memset_chk (buf3, '\0', 20, __builtin_object_size (buf3, 0));
  s5 = (char *) &a;
  __asm __volatile ("" : : "r" (s5) : "memory");
  s5 = buf3;
  __asm __volatile ("" : : "r" (s5) : "memory");
  chk_fail_allowed = 1;
  /* Runtime checks.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___strcat_chk (&a.buf2[9], s2 + 3, __builtin_object_size (&a.buf2[9], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___strcat_chk (&a.buf2[7], s3 + strlen (s3) - 3, __builtin_object_size (&a.buf2[7], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___strcat_chk (&buf3[19], "a", __builtin_object_size (&buf3[19], 0));
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
  s4 = p;
  test1 ();
  __builtin___memset_chk (p, '\0', 32, __builtin_object_size (p, 0));
  test2 ();
  test3 ();
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
