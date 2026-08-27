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

typedef long unsigned int size_t;
__attribute__ ((__noinline__))
int
strncmp(const char *s1, const char *s2, size_t n)
{
  const unsigned char *u1 = (const unsigned char *)s1;
  const unsigned char *u2 = (const unsigned char *)s2;
  unsigned char c1, c2;
  while (n > 0)
    {
      c1 = *u1++, c2 = *u2++;
      if (c1 == '\0' || c1 != c2)
 return c1 - c2;
      n--;
    }
  return c1 - c2;
}
extern void abort (void);
typedef long unsigned int size_t;
extern int strncmp (const char *, const char *, size_t);
void
main_test (void)
{
  const char *const s1 = "hello world";
  const char *s2;
  int n = 6, x;
  s2 = s1;
  if (strncmp (++s2, "ello", 3) != 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("ello", ++s2, 3) != 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "ello", 4) != 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("ello", ++s2, 4) != 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "ello", 5) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("ello", ++s2, 5) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "ello", 6) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("ello", ++s2, 6) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "zllo", 3) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("zllo", ++s2, 3) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "zllo", 4) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("zllo", ++s2, 4) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "zllo", 5) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("zllo", ++s2, 5) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "zllo", 6) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("zllo", ++s2, 6) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "allo", 3) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("allo", ++s2, 3) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "allo", 4) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("allo", ++s2, 4) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "allo", 5) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("allo", ++s2, 5) >= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp (++s2, "allo", 6) <= 0 || s2 != s1+1)
    abort();
  s2 = s1;
  if (strncmp ("allo", ++s2, 6) >= 0 || s2 != s1+1)
    abort();
  s2 = s1; n = 2; x = 1;
  if (strncmp (++s2, s1+(x&3), ++n) != 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 2; x = 1;
  if (strncmp (s1+(x&3), ++s2, ++n) != 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp (++s2, s1+(x&3), ++n) != 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp (s1+(x&3), ++s2, ++n) != 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp (++s2, s1+(x&3), ++n) != 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp (s1+(x&3), ++s2, ++n) != 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp (++s2, s1+(x&3), ++n) != 0 || s2 != s1+1 || n != 6)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp (s1+(x&3), ++s2, ++n) != 0 || s2 != s1+1 || n != 6)
    abort();
  s2 = s1; n = 2;
  if (strncmp (++s2, "zllo", ++n) >= 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 2; x = 1;
  if (strncmp ("zllo", ++s2, ++n) <= 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp (++s2, "zllo", ++n) >= 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp ("zllo", ++s2, ++n) <= 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp (++s2, "zllo", ++n) >= 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp ("zllo", ++s2, ++n) <= 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp (++s2, "zllo", ++n) >= 0 || s2 != s1+1 || n != 6)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp ("zllo", ++s2, ++n) <= 0 || s2 != s1+1 || n != 6)
    abort();
  s2 = s1; n = 2;
  if (strncmp (++s2, "allo", ++n) <= 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 2; x = 1;
  if (strncmp ("allo", ++s2, ++n) >= 0 || s2 != s1+1 || n != 3)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp (++s2, "allo", ++n) <= 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 3; x = 1;
  if (strncmp ("allo", ++s2, ++n) >= 0 || s2 != s1+1 || n != 4)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp (++s2, "allo", ++n) <= 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 4; x = 1;
  if (strncmp ("allo", ++s2, ++n) >= 0 || s2 != s1+1 || n != 5)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp (++s2, "allo", ++n) <= 0 || s2 != s1+1 || n != 6)
    abort();
  s2 = s1; n = 5; x = 1;
  if (strncmp ("allo", ++s2, ++n) >= 0 || s2 != s1+1 || n != 6)
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
