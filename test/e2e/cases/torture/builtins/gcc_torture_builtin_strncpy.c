// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strncpy.c (+ lib/strncpy.c + lib/main.c).
 * RESET_DST is expanded. Original abort checks are kept. Local strncpy follows
 * ISO C / glibc: copy at most n chars, then zero-pad the remainder. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);

int i;

__attribute__ ((__noinline__))
char *
strncpy(char *s1, const char *s2, size_t n)
{
  char *dest = s1;
  while (*s2 && n) {
    n--;
    *s1++ = *s2++;
  }
  while (n) {
    n--;
    *s1++ = 0;
  }
  return dest;
}

extern int memcmp (const void *, const void *, size_t);
extern void *memset (void *, int, size_t);

void
main_test (void)
{
  const char *const src = "hello world";
  const char *src2;
  char dst[64], *dst2;

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst, src, 4) != dst || memcmp (dst, "hellXXX", 7))
    abort();

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst+16, src, 4) != dst+16 || memcmp (dst+16, "hellXXX", 7))
    abort();

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst+32, src+5, 4) != dst+32 || memcmp (dst+32, " worXXX", 7))
    abort();

  memset(dst, 'X', sizeof(dst));
  dst2 = dst;
  if (strncpy (++dst2, src+5, 4) != dst+1 || memcmp (dst2, " worXXX", 7)
      || dst2 != dst+1)
    abort();

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst, src, 0) != dst || memcmp (dst, "XXX", 3))
    abort();

  memset(dst, 'X', sizeof(dst));
  dst2 = dst; src2 = src;
  if (strncpy (++dst2, ++src2, 0) != dst+1 || memcmp (dst2, "XXX", 3)
      || dst2 != dst+1 || src2 != src+1)
    abort();

  memset(dst, 'X', sizeof(dst));
  dst2 = dst; src2 = src;
  if (strncpy (++dst2+5, ++src2+5, 0) != dst+6 || memcmp (dst2+5, "XXX", 3)
      || dst2 != dst+1 || src2 != src+1)
    abort();

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst, src, 12) != dst || memcmp (dst, "hello world\0XXX", 15))
    abort();

  memset(dst, 'X', sizeof(dst));
  if (__builtin_strncpy (dst, src, 4) != dst || memcmp (dst, "hellXXX", 7))
    abort();

  memset(dst, 'X', sizeof(dst));
  if (strncpy (dst, i++ ? "xfoo" + 1 : "bar", 4) != dst
      || memcmp (dst, "bar\0XXX", 7)
      || i != 1)
    abort ();
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
