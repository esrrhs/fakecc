// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strspn.c (+ lib/main.c).
 * Original abort checks are kept. Local strspn follows ISO C / glibc. */

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
size_t
strspn (const char *s, const char *accept)
{
  size_t i = 0;
  while (s[i] != 0) {
    const char *p = accept;
    int found = 0;
    while (*p != 0) {
      if (s[i] == *p) {
        found = 1;
        break;
      }
      p = p + 1;
    }
    if (found == 0)
      break;
    i = i + 1;
  }
  return i;
}

extern char *strcpy (char *, const char *);

void
main_test (void)
{
  const char *const s1 = "hello world";
  char dst[64], *d2;

  if (strspn (s1, "hello") != 5)
    abort();
  if (strspn (s1+4, "hello") != 1)
    abort();
  if (strspn (s1, "z") != 0)
    abort();
  if (strspn (s1, "hello world") != 11)
    abort();
  if (strspn (s1, "") != 0)
    abort();
  strcpy (dst, s1);
  if (strspn (dst, "") != 0)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strspn (++d2, "") != 0 || d2 != dst+1)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strspn (++d2+5, "") != 0 || d2 != dst+1)
    abort();
  if (strspn ("", s1) != 0)
    abort();
  strcpy (dst, s1);
  if (strspn ("", dst) != 0)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strspn ("", ++d2) != 0 || d2 != dst+1)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strspn ("", ++d2+5) != 0 || d2 != dst+1)
    abort();

  if (__builtin_strspn (s1, "hello") != 5)
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
