// expect: 0
// flags: -O1
package main;

/* Port of gcc.c-torture/execute/builtins/memset-chk.c (+ chk.h + lib/chk.c +
 * lib/main.c). chk.h memset macros are expanded to
 * __builtin___memset_chk(..., __builtin_object_size(dst, 0)). GCC lib
 * inside_main/__OPTIMIZE__ abort-on-memset is omitted (optimizer-only).
 * test3's struct tag is A3: fakecc rejects two block-scope structs named A.
 * GCC 16 DCE of memsets into dead locals needs chk_escape so chk_calls stay
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

/* GCC 16+ DCE's memset into dead locals, which also drops __memset_chk /
 * chk_calls.  Escape destinations so the original abort checks stay live. */
volatile char *volatile chk_escape;

char buffer[32];
int argc = 1;
volatile size_t l1 = 1;  /* prevent constant propagation to happen when whole program assumptions are made.  */
volatile char *s3 = "FGH"; /* prevent constant propagation to happen when whole program assumptions are made.  */
char *s4;

void
__attribute__((noinline))
test1 (void)
{
  memset_disallowed = 1;
  chk_calls = 0;
  __builtin___memset_chk (buffer, argc, 0, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 1, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 2, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 3, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 4, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 5, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 6, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 7, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 8, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 9, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 10, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 11, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 12, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 13, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 14, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 15, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 16, __builtin_object_size (buffer, 0));
  __builtin___memset_chk (buffer, argc, 17, __builtin_object_size (buffer, 0));
  memset_disallowed = 0;
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
  size_t l;
  chk_escape = a.buf1;
  chk_escape = buf3;

  /* The following calls should do runtime checking
     - length is not known, but destination is.  */
  chk_calls = 0;
  __builtin___memset_chk (a.buf1 + 2, 'a', l1, __builtin_object_size (a.buf1 + 2, 0));
  __builtin___memset_chk (r, '\0', l1 + 1, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___memset_chk (r, argc, l1 + 2, __builtin_object_size (r, 0));
  __builtin___memset_chk (r + 2, 'Q', l1, __builtin_object_size (r + 2, 0));
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
  __builtin___memset_chk (r, '\0', l1, __builtin_object_size (r, 0));
  if (chk_calls != 5)
    abort ();

  /* Following have known destination and known length,
     so if optimizing certainly shouldn't result in the checking
     variants.  */
  chk_calls = 0;
  __builtin___memset_chk (a.buf1 + 2, '\0', 1, __builtin_object_size (a.buf1 + 2, 0));
  __builtin___memset_chk (r, argc, 2, __builtin_object_size (r, 0));
  r = l1 == 1 ? __builtin_alloca (4) : &a.buf2[7];
  __builtin___memset_chk (r, 'N', 3, __builtin_object_size (r, 0));
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
  __builtin___memset_chk (r, 'H', 1, __builtin_object_size (r, 0));
  /* Here, l is known to be at most 4 and __builtin_object_size (&buf3[16], 0)
     is 4, so this doesn't need runtime checking.  */
  __builtin___memset_chk (&buf3[16], 'd', l, __builtin_object_size (&buf3[16], 0));
  /* Neither length nor destination known.  Doesn't need runtime checking.  */
  __builtin___memset_chk (s4, 'a', l1, __builtin_object_size (s4, 0));
  __builtin___memset_chk (s4 + 2, '\0', l1 + 2, __builtin_object_size (s4 + 2, 0));
  /* Destination unknown.  */
  __builtin___memset_chk (s4 + 4, 'b', 2, __builtin_object_size (s4 + 4, 0));
  __builtin___memset_chk (s4 + 6, '\0', 4, __builtin_object_size (s4 + 6, 0));
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

  chk_fail_allowed = 1;
  /* Runtime checks.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___memset_chk (&a.buf2[9], '\0', l1 + 1, __builtin_object_size (&a.buf2[9], 0));
      abort ();
    }
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___memset_chk (&a.buf2[7], 'T', strlen (s3) + 1, __builtin_object_size (&a.buf2[7], 0));
      abort ();
    }
  /* This should be detectable at compile time already.  */
  if (__builtin_setjmp (chk_fail_buf) == 0)
    {
      __builtin___memset_chk (&buf3[19], 'b', 2, __builtin_object_size (&buf3[19], 0));
      abort ();
    }
  chk_fail_allowed = 0;
}

static union {
  char buf[96];
  long long align_int;
  long double align_fp;
} u;

char A = 'A';

void
__attribute__((noinline))
test4 (void)
{
  int off, len, i;
  char *p, *q;

  for (off = 0; off < 8; off++)
    for (len = 1; len < 80; len++)
      {
	for (i = 0; i < 96; i++)
	  u.buf[i] = 'a';

	p = __builtin___memset_chk (u.buf + off, '\0', len, __builtin_object_size (u.buf + off, 0));
	if (p != u.buf + off)
	  abort ();

	q = u.buf;
	for (i = 0; i < off; i++, q++)
	  if (*q != 'a')
	    abort ();

	for (i = 0; i < len; i++, q++)
	  if (*q != '\0')
	    abort ();

	for (i = 0; i < 8; i++, q++)
	  if (*q != 'a')
	    abort ();

	p = __builtin___memset_chk (u.buf + off, A, len, __builtin_object_size (u.buf + off, 0));
	if (p != u.buf + off)
	  abort ();

	q = u.buf;
	for (i = 0; i < off; i++, q++)
	  if (*q != 'a')
	    abort ();

	for (i = 0; i < len; i++, q++)
	  if (*q != 'A')
	    abort ();

	for (i = 0; i < 8; i++, q++)
	  if (*q != 'a')
	    abort ();

	p = __builtin___memset_chk (u.buf + off, 'B', len, __builtin_object_size (u.buf + off, 0));
	if (p != u.buf + off)
	  abort ();

	q = u.buf;
	for (i = 0; i < off; i++, q++)
	  if (*q != 'a')
	    abort ();

	for (i = 0; i < len; i++, q++)
	  if (*q != 'B')
	    abort ();

	for (i = 0; i < 8; i++, q++)
	  if (*q != 'a')
	    abort ();
      }
}

static union {
  char buf[31];
  long long align_int;
  long double align_fp;
} u2;

void reset (void)
{
  int i;

  for (i = 0; i < 31; i++)
    u2.buf[i] = 'a';
}

void check (int off, int len, int ch)
{
  char *q;
  int i;

  q = u2.buf;
  for (i = 0; i < off; i++, q++)
    if (*q != 'a')
      abort ();

  for (i = 0; i < len; i++, q++)
    if (*q != ch)
      abort ();

  for (i = 0; i < 8; i++, q++)
    if (*q != 'a')
      abort ();
}

void
__attribute__((noinline))
test5 (void)
{
  int off;
  char *p;

  /* len == 1 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 1, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 1, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 1, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 1, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 1, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 1, 'B');
    }

  /* len == 2 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 2, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 2, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 2, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 2, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 2, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 2, 'B');
    }

  /* len == 3 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 3, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 3, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 3, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 3, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 3, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 3, 'B');
    }

  /* len == 4 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 4, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 4, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 4, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 4, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 4, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 4, 'B');
    }

  /* len == 5 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 5, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 5, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 5, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 5, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 5, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 5, 'B');
    }

  /* len == 6 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 6, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 6, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 6, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 6, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 6, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 6, 'B');
    }

  /* len == 7 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 7, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 7, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 7, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 7, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 7, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 7, 'B');
    }

  /* len == 8 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 8, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 8, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 8, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 8, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 8, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 8, 'B');
    }

  /* len == 9 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 9, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 9, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 9, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 9, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 9, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 9, 'B');
    }

  /* len == 10 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 10, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 10, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 10, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 10, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 10, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 10, 'B');
    }

  /* len == 11 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 11, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 11, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 11, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 11, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 11, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 11, 'B');
    }

  /* len == 12 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 12, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 12, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 12, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 12, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 12, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 12, 'B');
    }

  /* len == 13 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 13, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 13, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 13, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 13, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 13, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 13, 'B');
    }

  /* len == 14 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 14, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 14, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 14, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 14, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 14, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 14, 'B');
    }

  /* len == 15 */
  for (off = 0; off < 8; off++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf + off, '\0', 15, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 15, '\0');

      p = __builtin___memset_chk (u2.buf + off, A, 15, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 15, 'A');

      p = __builtin___memset_chk (u2.buf + off, 'B', 15, __builtin_object_size (u2.buf + off, 0));
      if (p != u2.buf + off) abort ();
      check (off, 15, 'B');
    }
}

void
__attribute__((noinline))
test6 (void)
{
  int len;
  char *p;

  /* off == 0 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf, '\0', len, __builtin_object_size (u2.buf, 0));
      if (p != u2.buf) abort ();
      check (0, len, '\0');

      p = __builtin___memset_chk (u2.buf, A, len, __builtin_object_size (u2.buf, 0));
      if (p != u2.buf) abort ();
      check (0, len, 'A');

      p = __builtin___memset_chk (u2.buf, 'B', len, __builtin_object_size (u2.buf, 0));
      if (p != u2.buf) abort ();
      check (0, len, 'B');
    }

  /* off == 1 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+1, '\0', len, __builtin_object_size (u2.buf+1, 0));
      if (p != u2.buf+1) abort ();
      check (1, len, '\0');

      p = __builtin___memset_chk (u2.buf+1, A, len, __builtin_object_size (u2.buf+1, 0));
      if (p != u2.buf+1) abort ();
      check (1, len, 'A');

      p = __builtin___memset_chk (u2.buf+1, 'B', len, __builtin_object_size (u2.buf+1, 0));
      if (p != u2.buf+1) abort ();
      check (1, len, 'B');
    }

  /* off == 2 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+2, '\0', len, __builtin_object_size (u2.buf+2, 0));
      if (p != u2.buf+2) abort ();
      check (2, len, '\0');

      p = __builtin___memset_chk (u2.buf+2, A, len, __builtin_object_size (u2.buf+2, 0));
      if (p != u2.buf+2) abort ();
      check (2, len, 'A');

      p = __builtin___memset_chk (u2.buf+2, 'B', len, __builtin_object_size (u2.buf+2, 0));
      if (p != u2.buf+2) abort ();
      check (2, len, 'B');
    }

  /* off == 3 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+3, '\0', len, __builtin_object_size (u2.buf+3, 0));
      if (p != u2.buf+3) abort ();
      check (3, len, '\0');

      p = __builtin___memset_chk (u2.buf+3, A, len, __builtin_object_size (u2.buf+3, 0));
      if (p != u2.buf+3) abort ();
      check (3, len, 'A');

      p = __builtin___memset_chk (u2.buf+3, 'B', len, __builtin_object_size (u2.buf+3, 0));
      if (p != u2.buf+3) abort ();
      check (3, len, 'B');
    }

  /* off == 4 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+4, '\0', len, __builtin_object_size (u2.buf+4, 0));
      if (p != u2.buf+4) abort ();
      check (4, len, '\0');

      p = __builtin___memset_chk (u2.buf+4, A, len, __builtin_object_size (u2.buf+4, 0));
      if (p != u2.buf+4) abort ();
      check (4, len, 'A');

      p = __builtin___memset_chk (u2.buf+4, 'B', len, __builtin_object_size (u2.buf+4, 0));
      if (p != u2.buf+4) abort ();
      check (4, len, 'B');
    }

  /* off == 5 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+5, '\0', len, __builtin_object_size (u2.buf+5, 0));
      if (p != u2.buf+5) abort ();
      check (5, len, '\0');

      p = __builtin___memset_chk (u2.buf+5, A, len, __builtin_object_size (u2.buf+5, 0));
      if (p != u2.buf+5) abort ();
      check (5, len, 'A');

      p = __builtin___memset_chk (u2.buf+5, 'B', len, __builtin_object_size (u2.buf+5, 0));
      if (p != u2.buf+5) abort ();
      check (5, len, 'B');
    }

  /* off == 6 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+6, '\0', len, __builtin_object_size (u2.buf+6, 0));
      if (p != u2.buf+6) abort ();
      check (6, len, '\0');

      p = __builtin___memset_chk (u2.buf+6, A, len, __builtin_object_size (u2.buf+6, 0));
      if (p != u2.buf+6) abort ();
      check (6, len, 'A');

      p = __builtin___memset_chk (u2.buf+6, 'B', len, __builtin_object_size (u2.buf+6, 0));
      if (p != u2.buf+6) abort ();
      check (6, len, 'B');
    }

  /* off == 7 */
  for (len = 0; len < 15; len++)
    {
      reset ();

      p = __builtin___memset_chk (u2.buf+7, '\0', len, __builtin_object_size (u2.buf+7, 0));
      if (p != u2.buf+7) abort ();
      check (7, len, '\0');

      p = __builtin___memset_chk (u2.buf+7, A, len, __builtin_object_size (u2.buf+7, 0));
      if (p != u2.buf+7) abort ();
      check (7, len, 'A');

      p = __builtin___memset_chk (u2.buf+7, 'B', len, __builtin_object_size (u2.buf+7, 0));
      if (p != u2.buf+7) abort ();
      check (7, len, 'B');
    }
}

void
main_test (void)
{
  __asm ("" : "=r" (l1) : "0" (l1));
  s4 = buffer;
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
