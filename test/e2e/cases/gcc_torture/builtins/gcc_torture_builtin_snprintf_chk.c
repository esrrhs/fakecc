// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/snprintf-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h snprintf/memset macros are expanded to
 * __builtin___*_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-snprintf is omitted (optimizer-only).
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
extern int memcmp (const void *, const void *, size_t);
extern size_t strlen (const char *);
extern int snprintf (char *, size_t, const char *, ...);
extern int vsnprintf (char *, size_t, const char *, va_list);
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

int
__snprintf_chk (char *str, size_t len, int flag, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;
  if (size == (size_t) -1 && flag == 0)
    abort ();
  ++chk_calls;
  if (size < len)
    __chk_fail ();
  va_start (ap, fmt);
  ret = vsnprintf (str, len, fmt, ap);
  va_end (ap);
  return ret;
}

/* GCC 16+ DCE of snprintf/memset into dead locals also drops chk_calls.
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
  /* snprintf_disallowed = 1; */

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___snprintf_chk (buffer, 4, 0, __builtin_object_size (buffer, 0), "foo");
  if (memcmp (buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___snprintf_chk (buffer, 4, 0, __builtin_object_size (buffer, 0), "foo bar") != 7)
    abort ();
  if (memcmp (buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___snprintf_chk (buffer, 32, 0, __builtin_object_size (buffer, 0), "%s", "bar");
  if (memcmp (buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___snprintf_chk (buffer, 21, 0, __builtin_object_size (buffer, 0), "%s", "bar") != 3)
    abort ();
  if (memcmp (buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  snprintf_disallowed = 0;

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___snprintf_chk (buffer, 4, 0, __builtin_object_size (buffer, 0), "%d%d%d", (int) l1, (int) l1 + 1, (int) l1 + 12)
      != 4)
    abort ();
  if (memcmp (buffer, "121", 4) || buffer[4] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  if (__builtin___snprintf_chk (buffer, 32, 0, __builtin_object_size (buffer, 0), "%d%d%d", (int) l1, (int) l1 + 1, (int) l1 + 12)
      != 4)
    abort ();
  if (memcmp (buffer, "1213", 5) || buffer[5] != 'A')
    abort ();

  if (chk_calls)
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___snprintf_chk (buffer, strlen (ptr) + 1, 0, __builtin_object_size (buffer, 0), "%s", ptr);
  if (memcmp (buffer, "barf", 5) || buffer[5] != 'A')
    abort ();

  __builtin___memset_chk (buffer, 'A', 32, __builtin_object_size (buffer, 0));
  __builtin___snprintf_chk (buffer, l1 + 31, 0, __builtin_object_size (buffer, 0), "%d - %c", (int) l1 + 27, *ptr);
  if (memcmp (buffer, "28 - b\0AAAAA", 12))
    abort ();

  if (chk_calls != 2)
    abort ();
  chk_calls = 0;

  __builtin___memset_chk (s4, 'A', 32, __builtin_object_size (s4, 0));
  __builtin___snprintf_chk (s4, l1 + 6, 0, __builtin_object_size (s4, 0), "%d - %c", (int) l1 - 17, ptr[1]);
  if (memcmp (s4, "-16 - \0AAA", 10))
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
     - length is not known, but destination is.  */
  chk_calls = 0;
  __builtin___snprintf_chk (a.buf1 + 2, l1, 0, __builtin_object_size (a.buf1 + 2, 0), "%s", s3 + 3);
  __builtin___snprintf_chk (r, l1 + 4, 0, __builtin_object_size (r, 0), "%s%c", s3 + 3, s3[3]);
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___snprintf_chk (r, strlen (s2) - 2, 0, __builtin_object_size (r, 0), "%c %s", s2[2], s2 + 4);
  __builtin___snprintf_chk (r + 2, l1, 0, __builtin_object_size (r + 2, 0), s3 + 3);
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
  __builtin___snprintf_chk (r, l1, 0, __builtin_object_size (r, 0), s2 + 4);
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known source length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  /* snprintf_disallowed = 1; */
  __builtin___snprintf_chk (a.buf1 + 2, 4, 0, __builtin_object_size (a.buf1 + 2, 0), "");
  __builtin___snprintf_chk (r, 1, 0, __builtin_object_size (r, 0), "a");
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___snprintf_chk (r, 3, 0, __builtin_object_size (r, 0), "%s", s1 + 1);
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
  __builtin___snprintf_chk (r, 1, 0, __builtin_object_size (r, 0), "%s", "");
  __builtin___snprintf_chk (r, 0, 0, __builtin_object_size (r, 0), "%s", "");
  snprintf_disallowed = 0;
  /* Unknown destination and source, no checking.  */
  __builtin___snprintf_chk (s4, l1 + 31, 0, __builtin_object_size (s4, 0), "%s %d", s3, 0);
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
      __builtin___snprintf_chk (&a.buf2[9], l1 + 1, 0, __builtin_object_size (&a.buf2[9], 0), "%c%s", s2[3], s2 + 4);
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___snprintf_chk (&a.buf2[7], l1 + 30, 0, __builtin_object_size (&a.buf2[7], 0), "%s%c", s3 + strlen (s3) - 2, *s3);
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___snprintf_chk (&a.buf2[7], l1 + 3, 0, __builtin_object_size (&a.buf2[7], 0), "%d", (int) l1 + 9999);
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___snprintf_chk (&buf3[19], 2, 0, __builtin_object_size (&buf3[19], 0), "a");
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___snprintf_chk (&buf3[17], 4, 0, __builtin_object_size (&buf3[17], 0), "a");
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___snprintf_chk (&buf3[17], 4, 0, __builtin_object_size (&buf3[17], 0), "%s", "abc");
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
