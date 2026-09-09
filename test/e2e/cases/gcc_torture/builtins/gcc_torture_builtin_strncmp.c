// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strncmp.c (+ lib/strncmp.c + lib/main.c).
 * Original abort checks are kept. Local strncmp follows the GCC test lib
 * (unsigned char compare) and returns 0 for n==0, matching ISO C / glibc. */

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
int
strncmp(const char *s1, const char *s2, size_t n)
{
  const unsigned char *u1 = (const unsigned char *)s1;
  const unsigned char *u2 = (const unsigned char *)s2;
  unsigned char c1 = 0, c2 = 0;

  while (n > 0)
    {
      c1 = *u1++, c2 = *u2++;
      if (c1 == '\0' || c1 != c2)
	return c1 - c2;
      n--;
    }
  return c1 - c2;
}

void
main_test (void)
{
  const char *const s1 = "hello world";
  const char *s2, *s3;

  if (strncmp (s1, "hello world", 12) != 0)
    abort();
  if (strncmp ("hello world", s1, 12) != 0)
    abort();
  if (strncmp ("hello", "hello", 6) != 0)
    abort();
  if (strncmp ("hello", "hello", 2) != 0)
    abort();
  if (strncmp ("hello", "hello", 100) != 0)
    abort();
  if (strncmp (s1+10, "d", 100) != 0)
    abort();
  if (strncmp (10+s1, "d", 100) != 0)
    abort();
  if (strncmp ("d", s1+10, 1) != 0)
    abort();
  if (strncmp ("d", 10+s1, 1) != 0)
    abort();
  if (strncmp ("hello", "aaaaa", 100) <= 0)
    abort();
  if (strncmp ("aaaaa", "hello", 100) >= 0)
    abort();
  if (strncmp ("hello", "aaaaa", 1) <= 0)
    abort();
  if (strncmp ("aaaaa", "hello", 1) >= 0)
    abort();

  s2 = s1; s3 = s1+4;
  if (strncmp (++s2, ++s3, 0) != 0 || s2 != s1+1 || s3 != s1+5)
    abort();
  s2 = s1;
  if (strncmp (++s2, "", 1) <= 0 || s2 != s1+1)
    abort();
  if (strncmp ("", ++s2, 1) >= 0 || s2 != s1+2)
    abort();
  if (strncmp (++s2, "", 100) <= 0 || s2 != s1+3)
    abort();
  if (strncmp ("", ++s2, 100) >= 0 || s2 != s1+4)
    abort();
  if (strncmp (++s2+6, "", 100) != 0 || s2 != s1+5)
    abort();
  if (strncmp ("", ++s2+5, 100) != 0 || s2 != s1+6)
    abort();
  if (strncmp ("ozz", ++s2, 1) != 0 || s2 != s1+7)
    abort();
  if (strncmp (++s2, "rzz", 1) != 0 || s2 != s1+8)
    abort();
  s2 = s1; s3 = s1+4;
  if (strncmp (++s2, ++s3+2, 1) >= 0 || s2 != s1+1 || s3 != s1+5)
    abort();

  if (__builtin_strncmp ("hello", "a", 100) <= 0)
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
