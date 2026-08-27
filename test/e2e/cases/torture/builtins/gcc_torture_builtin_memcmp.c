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
extern void abort(void);

__attribute__ ((__noinline__))
int
memcmp (const void *s1, const void *s2, long unsigned int len)
{
  const unsigned char *sp1, *sp2;
  sp1 = s1;
  sp2 = s2;
  while (len != 0 && *sp1 == *sp2)
    sp1++, sp2++, len--;
  if (len == 0)
    return 0;
  return *sp1 - *sp2;
}
extern void abort (void);
typedef long unsigned int size_t;
extern int memcmp (const void *, const void *, size_t);
extern char *strcpy (char *, const char *);
extern void link_error (void);
void
main_test (void)
{
  char str[8];
  strcpy (str, "3141");
  if ( memcmp (str, str+2, 0) != 0 )
    abort ();
  if ( memcmp (str+1, str+3, 0) != 0 )
    abort ();
  if ( memcmp (str+1, str+3, 1) != 0 )
    abort ();
  if ( memcmp (str, str+2, 1) >= 0 )
    abort ();
  if ( memcmp (str+2, str, 1) <= 0 )
    abort ();
  if (memcmp ("abcd", "efgh", 4) >= 0)
    link_error ();
  if (memcmp ("abcd", "abcd", 4) != 0)
    link_error ();
  if (memcmp ("efgh", "abcd", 4) <= 0)
    link_error ();
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
