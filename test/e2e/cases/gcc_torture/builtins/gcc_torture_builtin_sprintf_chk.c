// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/sprintf-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h sprintf/memset macros are expanded to
 * __builtin___*_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-sprintf is omitted (optimizer-only).
 * test3's struct tag is A3: fakecc rejects two block-scope structs named A.
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
extern void *memcpy (void *, const void *, size_t);
extern int memcmp (const void *, const void *, size_t);
extern size_t strlen (const char *);
extern int sprintf (char *, const char *, ...);
extern int vsprintf (char *, const char *, va_list);
extern void __fakecc_va_copy(void *dst, void *src);

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

static char chk_sprintf_buf[4096];

int
__sprintf_chk (char *str, int flag, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;
  if (size == (size_t) -1 && flag == 0)
    abort ();
  ++chk_calls;
  va_start (ap, fmt);
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  va_end (ap);
  if (ret >= 0)
    {
      if (ret >= size)
	__chk_fail ();
      memcpy (str, chk_sprintf_buf, ret + 1);
    }
  return ret;
}

/* GCC 16+ DCE of sprintf/memset into dead locals also drops chk_calls.
 * Escape destinations so the original abort checks stay live. */
volatile char *volatile chk_escape;

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
char *s4;
size_t l1 = 1;
static char buffer[32];
char * volatile ptr = "barf"; /* prevent constant propagation to happen when whole program assumptions are made.  */

void
__attribute__((noinline))
test1 (void)
{
  chk_calls = 0;
  /* sprintf_disallowed = 1; */

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "foo");
  if (memcmp (buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "foo") != 3)
    abort ();
  if (memcmp (buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "%s", "bar");
  if (memcmp (buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "%s", "bar") != 3)
    abort ();
  if (memcmp (buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  if (chk_calls)
    abort ();
  sprintf_disallowed = 0;

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "%s", ptr);
  if (memcmp (buffer, "barf", 5) || buffer[5] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___sprintf_chk (buffer, 0, __builtin_object_size (buffer, 0), "%d - %c", (int) l1 + 27, *ptr);
  if (memcmp (buffer, "28 - b\0AAAAA", 12))
    abort ();

  if (chk_calls != 2)
    abort ();
  chk_calls = 0;

  __builtin___sprintf_chk (s4, 0, __builtin_object_size (s4, 0), "%d - %c", (int) l1 - 17, ptr[1]);
  if (memcmp (s4, "-16 - a", 8))
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
  chk_calls = 0;
  __builtin___sprintf_chk (a.buf1 + 2, 0, __builtin_object_size (a.buf1 + 2, 0), "%s", s3 + 3);
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), "%s%c", s3 + 3, s3[3]);
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), "%c %s", s2[2], s2 + 4);
  __builtin___sprintf_chk (r + 2, 0, __builtin_object_size (r + 2, 0), s3 + 3);
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
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), s2 + 4);
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known source length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  /* sprintf_disallowed = 1; */
  __builtin___sprintf_chk (a.buf1 + 2, 0, __builtin_object_size (a.buf1 + 2, 0), "");
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), "a");
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), "%s", s1 + 1);
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
  __builtin___sprintf_chk (r, 0, __builtin_object_size (r, 0), "%s", "");
  sprintf_disallowed = 0;
  /* Unknown destination and source, no checking.  */
  __builtin___sprintf_chk (s4, 0, __builtin_object_size (s4, 0), "%s %d", s3, 0);
  if (chk_calls)
    abort ();
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

  chk_fail_allowed = 1;
  /* Runtime checks.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___sprintf_chk (&a.buf2[9], 0, __builtin_object_size (&a.buf2[9], 0), "%c%s", s2[3], s2 + 4);
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___sprintf_chk (&a.buf2[7], 0, __builtin_object_size (&a.buf2[7], 0), "%s%c", s3 + strlen (s3) - 2, *s3);
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___sprintf_chk (&a.buf2[7], 0, __builtin_object_size (&a.buf2[7], 0), "%d", (int) l1 + 9999);
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___sprintf_chk (&buf3[19], 0, __builtin_object_size (&buf3[19], 0), "a");
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___sprintf_chk (&buf3[17], 0, __builtin_object_size (&buf3[17], 0), "%s", "abc");
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
