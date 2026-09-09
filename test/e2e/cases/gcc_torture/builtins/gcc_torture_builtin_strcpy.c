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
extern void abort (void);

__attribute__ ((__noinline__))
char *
strcpy (char *d, const char *s)
{
  char *r = d;
  while ((*d++ = *s++));
  return r;
}
extern void abort (void);
extern char *strcpy (char *, const char *);
typedef long unsigned int size_t;
extern void *memcpy (void *, const void *, size_t);
extern int memcmp (const void *, const void *, size_t);
char p[32] = "";
void
main_test (void)
{
  if (strcpy (p, "abcde") != p || memcmp (p, "abcde", 6))
    abort ();
  if (strcpy (p + 16, "vwxyz" + 1) != p + 16 || memcmp (p + 16, "wxyz", 5))
    abort ();
  if (strcpy (p + 1, "") != p + 1 || memcmp (p, "a\0cde", 6))
    abort ();
  if (strcpy (p + 3, "fghij") != p + 3 || memcmp (p, "a\0cfghij", 9))
    abort ();
  if (memcpy (p, "ABCDE", 6) != p || memcmp (p, "ABCDE", 6))
    abort ();
  if (memcpy (p + 16, "VWX" + 1, 2) != p + 16 || memcmp (p + 16, "WXyz", 5))
    abort ();
  if (memcpy (p + 1, "", 1) != p + 1 || memcmp (p, "A\0CDE", 6))
    abort ();
  if (memcpy (p + 3, "FGHI", 4) != p + 3 || memcmp (p, "A\0CFGHIj", 9))
    abort ();
  if (__builtin_strcpy (p, "abcde") != p || memcmp (p, "abcde", 6))
    abort ();
  if (__builtin_memcpy (p, "ABCDE", 6) != p || memcmp (p, "ABCDE", 6))
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
