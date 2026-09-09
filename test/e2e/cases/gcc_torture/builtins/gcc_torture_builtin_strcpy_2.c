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
char buf[32], *p;
int i;
char *
__attribute__((noinline))
test (void)
{
  int j;
  const char *q = "abcdefg";
  for (j = 0; j < 3; ++j)
    {
      if (j == i)
        q = "bcdefgh";
      else if (j == i + 1)
        q = "cdefghi";
      else if (j == i + 2)
        q = "defghij";
    }
  p = strcpy (buf, q);
  return strcpy (buf + 16, q);
}
void
main_test (void)
{
  if (test () != buf + 16 || p != buf)
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
