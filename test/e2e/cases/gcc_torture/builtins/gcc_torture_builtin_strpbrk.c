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
strpbrk(const char *s1, const char *s2)
{
  const char *p;
  while (*s1)
    {
      for (p = s2; *p; p++)
 if (*s1 == *p)
   return (char *)s1;
      s1++;
    }
  return 0;
}
extern void abort(void);
extern char *strpbrk (const char *, const char *);
extern int strcmp (const char *, const char *);
void fn (const char *foo, const char *const *bar)
{
  if (strcmp(strpbrk ("hello world", "lrooo"), "llo world") != 0)
    abort();
  if (strpbrk (foo, "") != 0)
    abort();
  if (strpbrk (foo + 4, "") != 0)
    abort();
  if (strpbrk (*bar--, "") != 0)
    abort();
  if (strpbrk (*bar, "h") != foo)
    abort();
  if (strpbrk (foo, "h") != foo)
    abort();
  if (strpbrk (foo, "w") != foo + 6)
    abort();
  if (strpbrk (foo + 6, "o") != foo + 7)
    abort();
  if (__builtin_strpbrk (foo + 6, "o") != foo + 7)
    abort();
}
void
main_test (void)
{
  const char *const foo[] = { "hello world", "bye bye world" };
  fn (foo[0], foo + 1);
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
