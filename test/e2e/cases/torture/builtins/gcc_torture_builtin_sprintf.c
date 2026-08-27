// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/sprintf.c (+ lib/sprintf.c + lib/main.c).
 * Original abort checks on sprintf contents, trailing padding, and return
 * values are kept. */

int inside_main = 0;

extern int vprintf (const char *, __builtin_va_list);
typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);
extern void *memset(void*, int, size_t);
extern int memcmp(const void*, const void*, size_t);
extern int vsprintf(char *buf, const char *fmt, va_list ap);

static char buffer[32];

__attribute__ ((__noinline__))
int
sprintf (char *buf, const char *fmt, ...)
{
  va_list ap;
  int r;
  va_start (ap, fmt);
  r = vsprintf (buf, fmt, ap);
  va_end (ap);
  return r;
}

void test1()
{
  sprintf(buffer,"foo");
}

int test2()
{
  return sprintf(buffer,"foo");
}

void test3()
{
  sprintf(buffer,"%s","bar");
}

int test4()
{
  return sprintf(buffer,"%s","bar");
}

void test5(char *ptr)
{
  sprintf(buffer,"%s",ptr);
}

void
main_test (void)
{
  memset (buffer, 'A', 32);
  test1 ();
  if (memcmp(buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  memset (buffer, 'A', 32);
  if (test2 () != 3)
    abort ();
  if (memcmp(buffer, "foo", 4) || buffer[4] != 'A')
    abort ();

  memset (buffer, 'A', 32);
  test3 ();
  if (memcmp(buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  memset (buffer, 'A', 32);
  if (test4 () != 3)
    abort ();
  if (memcmp(buffer, "bar", 4) || buffer[4] != 'A')
    abort ();

  memset (buffer, 'A', 32);
  test5 ("barf");
  if (memcmp(buffer, "barf", 5) || buffer[5] != 'A')
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
