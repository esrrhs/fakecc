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
extern void abort (void);
extern long unsigned int strlen (const char *);
extern char *strcpy (char *, const char *);
static const char bar[] = "Hello, World!";
static const char baz[] = "hello, world?";
static const char larger[20] = "short string";

int l1 = 1;
int x = 6;
void
main_test(void)
{
  inside_main = 1;
  if (strlen (bar) != 13)
    abort ();
  if (strlen (bar + 3) != 10)
    abort ();
  if (strlen (&bar[6]) != 7)
    abort ();
  if (strlen (bar + (x++ & 7)) != 7)
    abort ();
  if (x != 7)
    abort ();
  if (strlen (larger) != 12)
    abort ();
  if (strlen (&larger[10]) != 2)
    abort ();
  inside_main = 0;
  if (strlen (larger + (x++ & 7)) != 5)
    abort ();
  if (x != 8)
    abort ();
  inside_main = 1;
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
