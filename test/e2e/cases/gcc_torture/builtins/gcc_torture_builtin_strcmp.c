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
int
strcmp (const char *s1, const char *s2)
{
  while (*s1 != 0 && *s1 == *s2)
    s1++, s2++;
  if (*s1 == 0 || *s2 == 0)
    return (unsigned char) *s1 - (unsigned char) *s2;
  return *s1 - *s2;
}
extern void abort (void);
extern int strcmp (const char *, const char *);
int x = 7;
char *bar = "hi world";
void
main_test (void)
{
  const char *const foo = "hello world";
  if (strcmp (foo, "hello") <= 0)
    abort ();
  if (strcmp (foo + 2, "llo") <= 0)
    abort ();
  if (strcmp (foo, foo) != 0)
    abort ();
  if (strcmp (foo, "hello world ") >= 0)
    abort ();
  if (strcmp (foo + 10, "dx") >= 0)
    abort ();
  if (strcmp (10 + foo, "dx") >= 0)
    abort ();
  if (strcmp (bar, "") <= 0)
    abort ();
  if (strcmp ("", bar) >= 0)
    abort ();
  if (strcmp (bar+8, "") != 0)
    abort ();
  if (strcmp ("", bar+8) != 0)
    abort ();
  if (strcmp (bar+(--x), "") <= 0 || x != 6)
    abort ();
  if (strcmp ("", bar+(++x)) >= 0 || x != 7)
    abort ();
  if (__builtin_strcmp (foo, "hello") <= 0)
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
