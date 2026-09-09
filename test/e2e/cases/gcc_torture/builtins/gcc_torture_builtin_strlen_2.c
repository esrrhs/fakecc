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
long unsigned int
strlen (const char *s)
{
  long unsigned int i;
  i = 0;
  while (s[i] != 0)
    i++;
  return i;
}
typedef long unsigned int size_t;
extern size_t strlen (const char *);
extern char *strcpy (char *, const char *);
extern int memcmp (const void *, const void *, size_t);
extern void abort (void);

size_t g, h, i, j, k, l;
size_t
foo (void)
{
  if (l)
    abort ();
  return ++l;
}
void
main_test (void)
{
  if (strlen (i ? "foo" + 1 : j ? "bar" + 1 : "baz" + 1) != 2)
    abort ();
  if (strlen (g++ ? "foo" : "bar") != 3 || g != 1)
    abort ();
  if (strlen (h++ ? "xfoo" + 1 : "bar") != 3 || h != 1)
    abort ();
  if (strlen ((i++, "baz")) != 3 || i != 1)
    abort ();
  inside_main = 0;
  if (strlen (j ? "foo" + k++ : "bar" + k++) != 3 || k != 1)
    abort ();
  if (strlen (foo () ? "foo" : "bar") != 3 || l != 1)
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
