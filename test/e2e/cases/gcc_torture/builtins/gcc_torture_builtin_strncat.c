// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strncat.c (+ lib/strncat.c + lib/main.c).
 * RESET_DST_WITH is expanded. Original abort checks are kept. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);

int x = 123;

__attribute__ ((__noinline__))
char *
strncat (char *s1, const char *s2, size_t n)
{
  char *dest = s1;
  char c = '\0';
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

extern char *strcpy (char *, const char *);
extern void *memset (void *, int, size_t);
extern int memcmp (const void *, const void *, size_t);

void
main_test (void)
{
  const char *const s1 = "hello world";
  const char *const s2 = "";
  char dst[64], *d2;

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strncat (dst, "", 100) != dst || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strncat (dst, s2, 100) != dst || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2, s2, 100) != dst+1 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, s2, 100) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, s1+11, 100) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, s1, 0) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, "", ++x) != dst+6 || d2 != dst+1 || x != 124
      || memcmp (dst, "hello world\0XXX", 15))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strncat (dst, "foo", 3) != dst || memcmp (dst, "hello worldfoo\0XXX", 18))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strncat (dst, "foo", 100) != dst || memcmp (dst, "hello worldfoo\0XXX", 18))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strncat (dst, s1, 100) != dst || memcmp (dst, "hello worldhello world\0XXX", 26))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2, s1, 100) != dst+1 || d2 != dst+1
      || memcmp (dst, "hello worldhello world\0XXX", 26))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, s1, 100) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello worldhello world\0XXX", 26))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strncat (++d2+5, s1+5, 100) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world world\0XXX", 21))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (__builtin_strncat (dst, "", 100) != dst
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
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
