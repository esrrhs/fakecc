// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strcat.c (+ lib/strcat.c + lib/main.c).
 * RESET_DST_WITH is expanded. The !__OPTIMIZE_SIZE__ block is kept (fakecc
 * does not compile at -Os). On x86_64 GCC leaves inside_main set, so the
 * corresponding inside_main = 0 path is not taken. */

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);

__attribute__ ((__noinline__))
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
  if (strcat (dst, "") != dst || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strcat (dst, s2) != dst || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strcat (++d2, s2) != dst+1 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strcat (++d2+5, s2) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();
  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strcat (++d2+5, s1+11) != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world\0XXX", 15))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strcat (dst, " 1111") != dst
      || memcmp (dst, "hello world 1111\0XXX", 20))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (strcat (dst+5, " 2222") != dst+5
      || memcmp (dst, "hello world 2222\0XXX", 20))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1)); d2 = dst;
  if (strcat (++d2+5, " 3333") != dst+6 || d2 != dst+1
      || memcmp (dst, "hello world 3333\0XXX", 20))
    abort();

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  strcat (strcat (strcat (strcat (strcat (strcat (dst, ": this "), ""),
				  "is "), "a "), "test"), ".");
  if (memcmp (dst, "hello world: this is a test.\0X", 30))
    abort();

  inside_main = 1;

  memset (dst, 'X', sizeof (dst)); strcpy (dst, (s1));
  if (__builtin_strcat (dst, "") != dst || memcmp (dst, "hello world\0XXX", 15))
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
