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
stpcpy (char *dst, const char *src)
{
  while (*src != 0)
    *dst++ = *src++;
  *dst = 0;
  return dst;
}
typedef long unsigned int size_t;
extern void abort (void);
extern char *strcpy (char *, const char *);
extern char *stpcpy (char *, const char *);
extern int memcmp (const void *, const void *, size_t);

const char s1[] = "123";
char p[32] = "";
char *s2 = "defg";
char *s3 = "FGH";
size_t l1 = 1;
void
main_test (void)
{
  int i = 8;
  if (stpcpy (p, "abcde") != p + 5 || memcmp (p, "abcde", 6))
    abort ();
  if (stpcpy (p + 16, "vwxyz" + 1) != p + 16 + 4 || memcmp (p + 16, "wxyz", 5))
    abort ();
  if (stpcpy (p + 1, "") != p + 1 + 0 || memcmp (p, "a\0cde", 6))
    abort ();
  if (stpcpy (p + 3, "fghij") != p + 3 + 5 || memcmp (p, "a\0cfghij", 9))
    abort ();
  if (stpcpy ((i++, p + 20 + 1), "23") != (p + 20 + 1 + 2)
      || i != 9 || memcmp (p + 19, "z\0""23\0", 5))
    abort ();
  if (stpcpy (stpcpy (p, "ABCD"), "EFG") != p + 7 || memcmp (p, "ABCDEFG", 8))
    abort();
  if (__builtin_stpcpy (p, "abcde") != p + 5 || memcmp (p, "abcde", 6))
    abort ();
  inside_main = 1;
  stpcpy (p + 3, s3);
  if (memcmp (p, "abcFGH", 6))
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
