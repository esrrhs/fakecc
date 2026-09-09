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
memmove (void *dst, const void *src, long unsigned int n)
{
  char *dstp;
  const char *srcp;
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
void
bcopy (const void *src, void *dst, long unsigned int n)
{
  memmove (dst, src, n);
}
extern void abort (void);
typedef long unsigned int size_t;
extern void *memmove (void *, const void *, size_t);
extern void bcopy (const void *, void *, size_t);
extern int memcmp (const void *, const void *, size_t);
char p[32] = "abcdefg";
char *q = p + 4;
void
main_test (void)
{
  if (memmove (p + 2, p + 3, 1) != p + 2 || memcmp (p, "abddefg", 8))
    abort ();
  if (memmove (p + 1, p + 1, 1) != p + 1 || memcmp (p, "abddefg", 8))
    abort ();
  if (memmove (q, p + 4, 1) != p + 4 || memcmp (p, "abddefg", 8))
    abort ();
  bcopy (p + 5, p + 6, 1);
  if (memcmp (p, "abddeff", 8))
    abort ();
  bcopy (p + 1, p + 1, 1);
  if (memcmp (p, "abddeff", 8))
    abort ();
  bcopy (q, p + 4, 1);
  if (memcmp (p, "abddeff", 8))
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
