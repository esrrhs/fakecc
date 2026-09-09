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
extern void abort (void);
typedef long unsigned int size_t;
extern size_t strlen(const char *);
extern void *memcpy (void *, const void *, size_t);
extern void *mempcpy (void *, const void *, size_t);
extern int memcmp (const void *, const void *, size_t);

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
size_t l1 = 1;
void
main_test (void)
{
  int i;
  if (mempcpy (p, "ABCDE", 6) != p + 6 || memcmp (p, "ABCDE", 6))
    abort ();
  if (mempcpy (p + 16, "VWX" + 1, 2) != p + 16 + 2
      || memcmp (p + 16, "WX\0\0", 5))
    abort ();
  if (mempcpy (p + 1, "", 1) != p + 1 + 1 || memcmp (p, "A\0CDE", 6))
    abort ();
  if (mempcpy (p + 3, "FGHI", 4) != p + 3 + 4 || memcmp (p, "A\0CFGHI", 8))
    abort ();
  i = 8;
  memcpy (p + 20, "qrstu", 6);
  memcpy (p + 25, "QRSTU", 6);
  if (mempcpy (p + 25 + 1, s1, 3) != (p + 25 + 1 + 3)
      || memcmp (p + 25, "Q123U", 6))
    abort ();
  if (mempcpy (mempcpy (p, "abcdEFG", 4), "efg", 4) != p + 8
      || memcmp (p, "abcdefg", 8))
    abort();
  if (__builtin_mempcpy (p, "ABCDE", 6) != p + 6 || memcmp (p, "ABCDE", 6))
    abort ();
  inside_main = 1;
  mempcpy (p + 5, s3, 1);
  if (memcmp (p, "ABCDEFg", 8))
    abort ();
  mempcpy (p + 6, s1 + 1, l1);
  if (memcmp (p, "ABCDEF2", 8))
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
