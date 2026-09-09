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
strstr(const char *s1, const char *s2)
{
  const char *p, *q;
  for (; *s1; s1++)
    {
      p = s1, q = s2;
      while (*q && *p)
 {
   if (*q != *p)
     break;
   p++, q++;
 }
      if (*q == 0)
 return (char *)s1;
    }
  return 0;
}
extern void abort(void);
extern char *strstr (const char *, const char *);
void
main_test (void)
{
  const char *const foo = "hello world";
  if (strstr (foo, "") != foo)
    abort();
  if (strstr (foo + 4, "") != foo + 4)
    abort();
  if (strstr (foo, "h") != foo)
    abort();
  if (strstr (foo, "w") != foo + 6)
    abort();
  if (strstr (foo + 6, "o") != foo + 7)
    abort();
  if (strstr (foo + 1, "world") != foo + 6)
    abort();
  if (__builtin_strstr (foo + 1, "world") != foo + 6)
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
