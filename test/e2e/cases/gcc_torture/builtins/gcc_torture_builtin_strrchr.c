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
strrchr (const char *s, int c)
{
  long unsigned int i;
  i = 0;
  while (s[i] != 0)
    i++;
  do
    if (s[i] == c)
      return (char *) s + i;
  while (i-- != 0);
  return 0;
}
__attribute__ ((__noinline__))
char *
rindex (const char *s, int c)
{
  return strrchr (s, c);
}
extern void abort (void);
extern char *strrchr (const char *, int);
extern char *rindex (const char *, int);
char *bar = "hi world";
int x = 7;
void
main_test (void)
{
  const char *const foo = "hello world";
  if (strrchr (foo, 'x'))
    abort ();
  if (strrchr (foo, 'o') != foo + 7)
    abort ();
  if (strrchr (foo, 'e') != foo + 1)
    abort ();
  if (strrchr (foo + 3, 'e'))
    abort ();
  if (strrchr (foo, '\0') != foo + 11)
    abort ();
  if (strrchr (bar, '\0') != bar + 8)
    abort ();
  if (strrchr (bar + 4, '\0') != bar + 8)
    abort ();
  if (strrchr (bar + (x++ & 3), '\0') != bar + 8)
    abort ();
  if (x != 8)
    abort ();
  if (rindex ("hello", 'z') != 0)
    abort ();
  if (__builtin_strrchr (foo, 'o') != foo + 7)
    abort ();
  if (__builtin_rindex (foo, 'o') != foo + 7)
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
