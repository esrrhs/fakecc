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
strchr (const char *s, int c)
{
  for (;;)
    {
      if (*s == c)
 return (char *) s;
      if (*s == 0)
 return 0;
      s++;
    }
}
__attribute__ ((__noinline__))
char *
index (const char *s, int c)
{
  return strchr (s, c);
}
extern void abort (void);
extern char *strchr (const char *, int);
extern char *index (const char *, int);
void
main_test (void)
{
  const char *const foo = "hello world";
  if (strchr (foo, 'x'))
    abort ();
  if (strchr (foo, 'o') != foo + 4)
    abort ();
  if (strchr (foo + 5, 'o') != foo + 7)
    abort ();
  if (strchr (foo, '\0') != foo + 11)
    abort ();
  if (index ("hello", 'z') != 0)
    abort ();
  if (__builtin_strchr (foo, 'o') != foo + 4)
    abort ();
  if (__builtin_index (foo, 'o') != foo + 4)
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
