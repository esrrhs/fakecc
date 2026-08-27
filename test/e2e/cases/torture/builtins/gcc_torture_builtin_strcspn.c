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
long unsigned int
strcspn (const char *s1, const char *s2)
{
  const char *p, *q;
  for (p = s1; *p; p++)
    for (q = s2; *q; q++)
      if (*p == *q)
 goto found;
 found:
  return p - s1;
}
extern void abort (void);
typedef long unsigned int size_t;
extern size_t strcspn (const char *, const char *);
extern char *strcpy (char *, const char *);
void
main_test (void)
{
  const char *const s1 = "hello world";
  char dst[64], *d2;
  if (strcspn (s1, "hello") != 0)
    abort();
  if (strcspn (s1, "z") != 11)
    abort();
  if (strcspn (s1+4, "z") != 7)
    abort();
  if (strcspn (s1, "hello world") != 0)
    abort();
  if (strcspn (s1, "") != 11)
    abort();
  strcpy (dst, s1);
  if (strcspn (dst, "") != 11)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strcspn (++d2, "") != 10 || d2 != dst+1)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strcspn (++d2+5, "") != 5 || d2 != dst+1)
    abort();
  if (strcspn ("", s1) != 0)
    abort();
  strcpy (dst, s1);
  if (strcspn ("", dst) != 0)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strcspn ("", ++d2) != 0 || d2 != dst+1)
    abort();
  strcpy (dst, s1); d2 = dst;
  if (strcspn ("", ++d2+5) != 0 || d2 != dst+1)
    abort();
  if (__builtin_strcspn (s1, "z") != 11)
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
