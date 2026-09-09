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
typedef long ssize_t;
extern void abort (void);

void *chk_fail_buf[256] __attribute__((aligned (16)));
volatile int chk_fail_allowed, chk_calls;
volatile int memcpy_disallowed, mempcpy_disallowed, memmove_disallowed;
volatile int memset_disallowed, strcpy_disallowed, stpcpy_disallowed;
volatile int strncpy_disallowed, stpncpy_disallowed, strcat_disallowed;
volatile int strncat_disallowed, sprintf_disallowed, vsprintf_disallowed;
volatile int snprintf_disallowed, vsnprintf_disallowed;
extern long unsigned int strlen (const char *);
extern int vsprintf (char *, const char *, va_list);
void __attribute__((noreturn))
__chk_fail (void)
{
  if (chk_fail_allowed)
    __builtin_longjmp (chk_fail_buf, 1);
  abort ();
}
void *
memcpy (void *dst, const void *src, long unsigned int n)
{
  const char *srcp;
  char *dstp;
  srcp = src;
  dstp = dst;
  while (n-- != 0)
    *dstp++ = *srcp++;
  return dst;
}
void *
__memcpy_chk (void *dst, const void *src, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memcpy (dst, src, n);
}
void *
mempcpy (void *dst, const void *src, long unsigned int n)
{
  const char *srcp;
  char *dstp;
  srcp = src;
  dstp = dst;
  while (n-- != 0)
    *dstp++ = *srcp++;
  return dstp;
}
void *
__mempcpy_chk (void *dst, const void *src, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return mempcpy (dst, src, n);
}
void *
memmove (void *dst, const void *src, long unsigned int n)
{
  const char *srcp;
  char *dstp;
  srcp = src;
  dstp = dst;
  if (srcp < dstp)
    while (n-- != 0)
      dstp[n] = srcp[n];
  else
    while (n-- != 0)
      *dstp++ = *srcp++;
  return dst;
}
void *
__memmove_chk (void *dst, const void *src, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memmove (dst, src, n);
}
void *
memset (void *dst, int c, long unsigned int n)
{
  while (n-- != 0)
    n[(char *) dst] = c;
  return dst;
}
void *
__memset_chk (void *dst, int c, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return memset (dst, c, n);
}
char *
strcpy (char *d, const char *s)
{
  char *r = d;
  while ((*d++ = *s++));
  return r;
}
char *
__strcpy_chk (char *d, const char *s, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (strlen (s) >= size)
    __chk_fail ();
  return strcpy (d, s);
}
char *
stpcpy (char *dst, const char *src)
{
  while (*src != 0)
    *dst++ = *src++;
  *dst = 0;
  return dst;
}
char *
__stpcpy_chk (char *d, const char *s, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (strlen (s) >= size)
    __chk_fail ();
  return stpcpy (d, s);
}
char *
stpncpy (char *dst, const char *src, long unsigned int n)
{
  for (; *src && n; n--)
    *dst++ = *src++;
  char *ret = dst;
  while (n--)
    *dst++ = 0;
  return ret;
}
char *
__stpncpy_chk (char *s1, const char *s2, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return stpncpy (s1, s2, n);
}
char *
strncpy (char *s1, const char *s2, long unsigned int n)
{
  char *dest = s1;
  for (; *s2 && n; n--)
    *s1++ = *s2++;
  while (n--)
    *s1++ = 0;
  return dest;
}
char *
__strncpy_chk (char *s1, const char *s2, long unsigned int n, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (n > size)
    __chk_fail ();
  return strncpy (s1, s2, n);
}
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
__strcat_chk (char *d, const char *s, long unsigned int size)
{
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  if (strlen (d) + strlen (s) >= size)
    __chk_fail ();
  return strcat (d, s);
}
char *
strncat (char *s1, const char *s2, long unsigned int n)
{
  char *dest = s1;
  char c;
  while (*s1) s1++;
  c = '\0';
  while (n > 0)
    {
      c = *s2++;
      *s1++ = c;
      if (c == '\0')
 return dest;
      n--;
    }
  if (c != '\0')
    *s1 = '\0';
  return dest;
}
char *
__strncat_chk (char *d, const char *s, long unsigned int n, long unsigned int size)
{
  long unsigned int len = strlen (d), n1 = n;
  const char *s1 = s;
  if (size == (long unsigned int) -1)
    abort ();
  ++chk_calls;
  while (len < size && n1 > 0)
    {
      if (*s1++ == '\0')
 break;
      ++len;
      --n1;
    }
  if (len >= size)
    __chk_fail ();
  return strncat (d, s, n);
}
static char chk_sprintf_buf[4096];
int
__sprintf_chk (char *str, int flag, long unsigned int size, const char *fmt, ...)
{
  int ret;
  va_list ap;
  if (size == (long unsigned int) -1 && flag == 0)
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
int
__vsprintf_chk (char *str, int flag, long unsigned int size, const char *fmt,
  va_list ap)
{
  int ret;
  if (size == (long unsigned int) -1 && flag == 0)
    abort ();
  ++chk_calls;
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  if (ret >= 0)
    {
      if (ret >= size)
 __chk_fail ();
      memcpy (str, chk_sprintf_buf, ret + 1);
    }
  return ret;
}
int
__snprintf_chk (char *str, long unsigned int len, int flag, long unsigned int size,
  const char *fmt, ...)
{
  int ret;
  va_list ap;
  if (size == (long unsigned int) -1 && flag == 0)
    abort ();
  ++chk_calls;
  if (size < len)
    __chk_fail ();
  va_start (ap, fmt);
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  va_end (ap);
  if (ret >= 0)
    {
      if (ret < len)
 memcpy (str, chk_sprintf_buf, ret + 1);
      else
 {
   memcpy (str, chk_sprintf_buf, len - 1);
   str[len - 1] = '\0';
 }
    }
  return ret;
}
int
__vsnprintf_chk (char *str, long unsigned int len, int flag, long unsigned int size,
   const char *fmt, va_list ap)
{
  int ret;
  if (size == (long unsigned int) -1 && flag == 0)
    abort ();
  ++chk_calls;
  if (size < len)
    __chk_fail ();
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  if (ret >= 0)
    {
      if (ret < len)
 memcpy (str, chk_sprintf_buf, ret + 1);
      else
 {
   memcpy (str, chk_sprintf_buf, len - 1);
   str[len - 1] = '\0';
 }
    }
  return ret;
}
int
snprintf (char *str, long unsigned int len, const char *fmt, ...)
{
  int ret;
  va_list ap;
  va_start (ap, fmt);
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  va_end (ap);
  if (ret >= 0)
    {
      if (ret < len)
 memcpy (str, chk_sprintf_buf, ret + 1);
      else if (len)
 {
   memcpy (str, chk_sprintf_buf, len - 1);
   str[len - 1] = '\0';
 }
    }
  return ret;
}
int
vsnprintf (char *str, long unsigned int len, const char *fmt, va_list ap)
{
  int ret;
  ret = vsprintf (chk_sprintf_buf, fmt, ap);
  if (ret >= 0)
    {
      if (ret < len)
 memcpy (str, chk_sprintf_buf, ret + 1);
      else if (len)
 {
   memcpy (str, chk_sprintf_buf, len - 1);
   str[len - 1] = '\0';
 }
    }
  return ret;
}
extern void abort (void);
typedef long unsigned int size_t;
extern size_t strlen (const char *);
extern void *memcpy (void *, const void *, size_t);
extern void *mempcpy (void *, const void *, size_t);
extern void *memmove (void *, const void *, size_t);
extern int snprintf (char *, size_t, const char *, ...);
extern int memcmp (const void *, const void *, size_t);
extern void *chk_fail_buf[];
extern volatile int chk_fail_allowed, chk_calls;
extern volatile int memcpy_disallowed, mempcpy_disallowed, memmove_disallowed;
extern volatile int memset_disallowed, strcpy_disallowed, stpcpy_disallowed;
extern volatile int strncpy_disallowed, stpncpy_disallowed, strcat_disallowed;
extern volatile int strncat_disallowed, sprintf_disallowed, vsprintf_disallowed;
extern volatile int snprintf_disallowed, vsnprintf_disallowed;
static char data[8] = "ABCDEFG";
int l1;
void
__attribute__((noinline))
test1 (void)
{
  char buf[8];
  chk_calls = 0;
  __builtin___memset_chk (buf, 'I', sizeof (buf), __builtin_object_size (buf, 0));
  if (__builtin___memcpy_chk (buf, data, l1 ? sizeof (buf) : 4, __builtin_object_size (buf, 0)) != buf
      || memcmp (buf, "ABCDIIII", 8))
    abort ();
  __builtin___memset_chk (buf, 'J', sizeof (buf), __builtin_object_size (buf, 0));
  if (__builtin___mempcpy_chk (buf, data, l1 ? sizeof (buf) : 4, __builtin_object_size (buf, 0)) != buf + 4
      || memcmp (buf, "ABCDJJJJ", 8))
    abort ();
  __builtin___memset_chk (buf, 'K', sizeof (buf), __builtin_object_size (buf, 0));
  if (__builtin___memmove_chk (buf, data, l1 ? sizeof (buf) : 4, __builtin_object_size (buf, 0)) != buf
      || memcmp (buf, "ABCDKKKK", 8))
    abort ();
  __builtin___memset_chk (buf, 'L', sizeof (buf), __builtin_object_size (buf, 0));
  if (__builtin___snprintf_chk (buf, l1 ? sizeof (buf) : 4, 0, __builtin_object_size (buf, 0), "%d", l1 + 65536) != 5
      || memcmp (buf, "655\0LLLL", 8))
    abort ();
  if (chk_calls)
    abort ();
}
void
main_test (void)
{
  return;
  __asm ("" : "=r" (l1) : "0" (l1));
  test1 ();
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
